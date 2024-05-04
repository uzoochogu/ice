#include "triangle_mesh.hpp"
namespace ice {
TriangleMesh::TriangleMesh(const vk::Device logical_device,
                           const vk::PhysicalDevice physical_device,
                           vk::Queue queue, vk::CommandBuffer command_buffer) {

  this->logical_device = logical_device;

  std::vector<Vertex> vertices = {{.pos = glm::vec3(0.0f, -0.05f, 0.0f),
                                   .color = glm::vec3(0.0f, 1.0f, 0.0f)},
                                  {.pos = glm::vec3(0.05f, 0.05f, 0.0f),
                                   .color = glm::vec3(0.0f, 1.0f, 0.0f)},
                                  {.pos = glm::vec3(-0.05f, 0.05f, 0.0f),
                                   .color = glm::vec3(0.0f, 1.0f, 0.0f)}};

  // clang-format off
	/* std::vector<float> vertices = { {
		 0.0f, -0.05f, 0.0f, 0.0f, 1.0f, 0.0f,
		 0.05f, 0.05f, 0.0f, 0.0f, 1.0f, 0.0f,
		-0.05f, 0.05f, 0.0f, 0.0f, 1.0f, 0.0f
	} }; */
  // clang-format on

  // staging buffer
  BufferCreationInput input_chunk{
      /* .size = sizeof(float) * vertices.size(), */
      .size = sizeof(Vertex) * vertices.size(),
      .usage = vk::BufferUsageFlagBits::eTransferSrc,
      .memory_properties = vk::MemoryPropertyFlagBits::eHostVisible |
                           vk::MemoryPropertyFlagBits::eHostCoherent,
      .logical_device = logical_device,
      .physical_device = physical_device,
  };

  BufferBundle staging_buffer = createBuffer(input_chunk);

  // the monitored memory location
  void *memory_location = logical_device.mapMemory(staging_buffer.buffer_memory,
                                                   0, input_chunk.size);
  memcpy(memory_location, vertices.data(), input_chunk.size);
  logical_device.unmapMemory(staging_buffer.buffer_memory);

  // vertex buffer
  input_chunk.usage = vk::BufferUsageFlagBits::eTransferDst |
                      vk::BufferUsageFlagBits::eVertexBuffer;
  input_chunk.memory_properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
  vertex_buffer = createBuffer(input_chunk);

  vk::Result result = copy_buffer(staging_buffer, vertex_buffer,
                                  input_chunk.size, queue, command_buffer);

  logical_device.destroyBuffer(staging_buffer.buffer);
  logical_device.freeMemory(staging_buffer.buffer_memory);
  if (result != vk::Result::eSuccess) {
    throw std::runtime_error(
        "Vertex copy operation and Triangle mesh creation failed!");
  }
}

TriangleMesh::~TriangleMesh() {
  logical_device.destroyBuffer(vertex_buffer.buffer);
  logical_device.freeMemory(vertex_buffer.buffer_memory);
}

} // namespace ice