#ifndef TRIANGLE_MESH
#define TRIANGLE_MESH

#include "config.hpp"
#include "data_buffers.hpp"
#include "mesh.hpp"

namespace ice {
// Holds a vertex buffer for a triangle mesh.
class TriangleMesh {
public:
  TriangleMesh(const vk::Device logical_device,
               const vk::PhysicalDevice physical_device, vk::Queue queue,
               vk::CommandBuffer command_buffer);
  ~TriangleMesh();
  BufferBundle vertex_buffer;

  std::uint32_t get_vertex_count() const;

private:
  vk::Device logical_device;
  std::vector<Vertex> vertices;
};
} // namespace ice

#endif