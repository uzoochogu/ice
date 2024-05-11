#include "mesh_collator.hpp"

namespace ice {

#ifndef NDEBUG
std::string to_string(MeshTypes type) {
  switch (type) {
  case MeshTypes::GROUND:
    return "Ground";
  case MeshTypes::GIRL:
    return "Girl";
  case MeshTypes::SKULL:
    return "Skull";
  }
  return "Invalid type";
}
#endif

void MeshCollator::consume(MeshTypes type,
                           const std::vector<Vertex> &vertex_data,
                           const std::vector<std::uint32_t> &index_data) {
  vertex_lump.reserve(vertex_data.size());
  index_lump.reserve(index_data.size());

  std::uint32_t vertex_count = static_cast<std::uint32_t>(vertex_data.size());

  index_lump_offsets.insert(
      std::make_pair(type, static_cast<int>(index_lump.size())));
  index_counts.insert(
      std::make_pair(type, static_cast<int>(index_data.size())));

#ifndef NDEBUG
  std::cout << std::format("\nMesh Type:        {:<8}, Vertex Count:  {}"
                           "\nTotal Attributes: {:<8}, Index count :  {}\n\n",
                           ice::to_string(type), vertex_count, vertex_count * 8,
                           static_cast<int>(index_data.size()));
#endif

  for (const Vertex &attribute : vertex_data) {
    vertex_lump.push_back(attribute);
  }
  for (std::uint32_t index : index_data) {
    index_lump.push_back(index_offset + index);
  }

  index_offset += vertex_count;
}
void MeshCollator::finalize(
    const VertexBufferFinalizationInput &finalization_chunk) {
  logical_device = finalization_chunk.logical_device;

  // make staging buffer for vertex
  BufferCreationInput input_chunk{
      .size = sizeof(Vertex) * vertex_lump.size(),
      .usage = vk::BufferUsageFlagBits::eTransferSrc,
      .memory_properties = vk::MemoryPropertyFlagBits::eHostVisible |
                           vk::MemoryPropertyFlagBits::eHostCoherent,
      .logical_device = finalization_chunk.logical_device,

      .physical_device = finalization_chunk.physical_device};
  BufferBundle staging_buffer = create_buffer(input_chunk);

  // fill
  void *memory_location = logical_device.mapMemory(staging_buffer.buffer_memory,
                                                   0, input_chunk.size);
  memcpy(memory_location, vertex_lump.data(), input_chunk.size);
  logical_device.unmapMemory(staging_buffer.buffer_memory);

  // make vertex buffer
  input_chunk.usage = vk::BufferUsageFlagBits::eTransferDst |
                      vk::BufferUsageFlagBits::eVertexBuffer;
  input_chunk.memory_properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
  vertex_buffer = create_buffer(input_chunk);

  // copy
  vk::Result result =
      copy_buffer(staging_buffer, vertex_buffer, input_chunk.size,
                  finalization_chunk.queue, finalization_chunk.command_buffer);
  if (result != vk::Result::eSuccess) {
    throw std::runtime_error(
        "Vertex copy operation and Triangle mesh creation failed!");
  }

  // destroy staging buffer
  logical_device.destroyBuffer(staging_buffer.buffer);
  logical_device.freeMemory(staging_buffer.buffer_memory);

  // make staging buffer for indices
  input_chunk.size = sizeof(std::uint32_t) * index_lump.size();
  input_chunk.usage = vk::BufferUsageFlagBits::eTransferSrc;
  input_chunk.memory_properties = vk::MemoryPropertyFlagBits::eHostVisible |
                                  vk::MemoryPropertyFlagBits::eHostCoherent;
  staging_buffer = create_buffer(input_chunk);

  // fill
  memory_location = logical_device.mapMemory(staging_buffer.buffer_memory, 0,
                                             input_chunk.size);
  memcpy(memory_location, index_lump.data(), input_chunk.size);
  logical_device.unmapMemory(staging_buffer.buffer_memory);

  // make index buffer
  input_chunk.usage = vk::BufferUsageFlagBits::eTransferDst |
                      vk::BufferUsageFlagBits::eIndexBuffer;
  input_chunk.memory_properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
  index_buffer = create_buffer(input_chunk);

  // copy
  result =
      copy_buffer(staging_buffer, index_buffer, input_chunk.size,
                  finalization_chunk.queue, finalization_chunk.command_buffer);

  // destroy staging buffer
  logical_device.destroyBuffer(staging_buffer.buffer);
  logical_device.freeMemory(staging_buffer.buffer_memory);

  // destroy resources
  vertex_lump.clear();
  index_lump.clear();

  if (result != vk::Result::eSuccess) {
    throw std::runtime_error(
        "Index copy operation and Triangle mesh creation failed!");
  }
}

MeshCollator::~MeshCollator() {
  logical_device.destroyBuffer(vertex_buffer.buffer);
  logical_device.freeMemory(vertex_buffer.buffer_memory);
  logical_device.destroyBuffer(index_buffer.buffer);
  logical_device.freeMemory(index_buffer.buffer_memory);
}
} // namespace ice