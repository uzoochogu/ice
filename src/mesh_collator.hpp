#ifndef MESH_COLLATOR
#define MESH_COLLATOR

#include "config.hpp"
#include "data_buffers.hpp"
#include "game_objects.hpp"
#include "mesh.hpp"

namespace ice {

struct VertexBufferFinalizationInput {
  vk::Device logical_device;
  vk::PhysicalDevice physical_device;
  vk::CommandBuffer command_buffer;
  vk::Queue queue;
};

// Collates multiple meshes and lump them into one vertex buffer (allocates it)
// stores useful attributes information of these meshes like offsets, vertex
// count etc.
class MeshCollator {
public:
  MeshCollator() = default;
  ~MeshCollator();
  void consume(MeshTypes type, const std::vector<Vertex> &vertex_data);
  void finalize(const VertexBufferFinalizationInput &finalization_chunk);
  BufferBundle vertex_buffer;
  std::unordered_map<MeshTypes, std::uint32_t> offsets;
  std::unordered_map<MeshTypes, std::uint32_t> sizes;

private:
  std::uint32_t offset{0};
  vk::Device logical_device;
  std::vector<Vertex> lump;
};

} // namespace ice

#endif