#include "mesh_collator.hpp"

namespace ice {

#ifndef NDEBUG
std::string to_string(MeshTypes type) {
  switch (type) {
  case MeshTypes::TRIANGLE:
    return "Triangle";
  case MeshTypes::SQUARE:
    return "Square";
  case MeshTypes::STAR:
    return "Star";
  }
  return "Invalid type";
}
#endif

void MeshCollator::consume(MeshTypes type,
                           const std::vector<Vertex> &vertex_data) {
  lump.reserve(vertex_data.size());
  for (const Vertex &attribute : vertex_data) {
    lump.push_back(attribute);
  }
  std::uint32_t vertex_count = static_cast<std::uint32_t>(vertex_data.size());

#ifndef NDEBUG
  std::cout << std::format("\nMesh Type: {}, Vertex Count:  {}\n",
                           ice::to_string(type), vertex_count);
#endif

  offsets.insert(std::make_pair(type, offset));
  sizes.insert(std::make_pair(type, vertex_count));

  offset += vertex_count;
}
void MeshCollator::finalize(
    const VertexBufferFinalizationInput &finalization_chunk) {
  logical_device = finalization_chunk.logical_device;

  BufferCreationInput input_chunk{
      .size = sizeof(Vertex) * lump.size(),
      .usage = vk::BufferUsageFlagBits::eTransferSrc,
      .memory_properties = vk::MemoryPropertyFlagBits::eHostVisible |
                           vk::MemoryPropertyFlagBits::eHostCoherent,
      .logical_device = finalization_chunk.logical_device,

      .physical_device = finalization_chunk.physical_device};

  BufferBundle staging_buffer = create_buffer(input_chunk);
  void *memory_location = logical_device.mapMemory(staging_buffer.buffer_memory,
                                                   0, input_chunk.size);
  memcpy(memory_location, lump.data(), input_chunk.size);
  logical_device.unmapMemory(staging_buffer.buffer_memory);

  // vertex buffer copy
  input_chunk.usage = vk::BufferUsageFlagBits::eTransferDst |
                      vk::BufferUsageFlagBits::eVertexBuffer;
  input_chunk.memory_properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
  vertex_buffer = create_buffer(input_chunk);
  vk::Result result =
      copy_buffer(staging_buffer, vertex_buffer, input_chunk.size,
                  finalization_chunk.queue, finalization_chunk.command_buffer);

  logical_device.destroyBuffer(staging_buffer.buffer);
  logical_device.freeMemory(staging_buffer.buffer_memory);
  if (result != vk::Result::eSuccess) {
    throw std::runtime_error(
        "Vertex copy operation and Triangle mesh creation failed!");
  }
}

MeshCollator::~MeshCollator() {
  logical_device.destroyBuffer(vertex_buffer.buffer);
  logical_device.freeMemory(vertex_buffer.buffer_memory);
}
} // namespace ice