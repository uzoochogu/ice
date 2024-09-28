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

  auto vertex_count = static_cast<std::uint32_t>(vertex_data.size());

  index_lump_offsets.insert(
      std::make_pair(type, static_cast<int>(index_lump.size())));
  index_counts.insert(
      std::make_pair(type, static_cast<int>(index_data.size())));

#ifndef NDEBUG
  std::cout << std::format(
      "\nMesh Type:        {:<8}, Vertex Count:  {}"
      "\nTotal Attributes: {:<8}, Index count :  {}\n\n",
      ice::to_string(type), vertex_count, vertex_count * 8,
      static_cast<int>(index_data.size()));
#endif

  for (const Vertex &attribute : vertex_data) {
    vertex_lump.push_back(attribute);
  }
  for (const std::uint32_t index : index_data) {
    index_lump.push_back(index_offset + index);
  }

  index_offset += vertex_count;
}

void MeshCollator::finalize(
    const VertexBufferFinalizationInput &finalization_chunk) {
  vertex_buffer = create_device_local_buffer(
      finalization_chunk.physical_device, finalization_chunk.logical_device,
      finalization_chunk.command_buffer, finalization_chunk.queue,
      vk::BufferUsageFlagBits::eVertexBuffer, vertex_lump);

  index_buffer = create_device_local_buffer(
      finalization_chunk.physical_device, finalization_chunk.logical_device,
      finalization_chunk.command_buffer, finalization_chunk.queue,
      vk::BufferUsageFlagBits::eIndexBuffer, index_lump);
  logical_device = finalization_chunk.logical_device;

  // destroy resources
  vertex_lump.clear();
  index_lump.clear();
}

MeshCollator::~MeshCollator() {
  logical_device.destroyBuffer(vertex_buffer.buffer);
  logical_device.freeMemory(vertex_buffer.buffer_memory);
  logical_device.destroyBuffer(index_buffer.buffer);
  logical_device.freeMemory(index_buffer.buffer_memory);
}
}  // namespace ice
