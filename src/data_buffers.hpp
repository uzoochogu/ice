#ifndef DATA_BUFFERS_HPP
#define DATA_BUFFERS_HPP
#include "config.hpp"

namespace ice {

struct BufferCreationInput {
  size_t size;
  vk::BufferUsageFlags usage;
  vk::MemoryPropertyFlags memory_properties{
      vk::MemoryPropertyFlagBits::eHostVisible |
          vk::MemoryPropertyFlagBits::eHostCoherent,
  };
  vk::Device logical_device;
  vk::PhysicalDevice physical_device;
};

// holds a vulkan buffer and memory allocation
struct BufferBundle {
  vk::Buffer buffer;
  vk::DeviceMemory buffer_memory;
};

/**
 * Finds a suitable memory type for buffers e.g. Vertex buffer.
 * @exception throws a runtime error if it fails.
 * @param  physical_device physical device to be queried.
 * @param  supported_memory_indices  bit field stores memory
                                  types that are supported.
 * @param requested_properties requested properties we want.
 */
inline std::uint32_t
find_memory_type_index(const vk::PhysicalDevice physical_device,
                       std::uint32_t supported_memory_indices,
                       vk::MemoryPropertyFlags requested_properties) {
  vk::PhysicalDeviceMemoryProperties mem_properties =
      physical_device.getMemoryProperties();

  for (std::uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
    bool supported{static_cast<bool>(supported_memory_indices &
                                     (1 << i))}; // supported if set
    bool sufficient{(mem_properties.memoryTypes[i].propertyFlags &
                     requested_properties) == requested_properties};
    if (supported && sufficient) {
      return i;
    }
  }
  throw std::runtime_error("failed to find suitable memory type!");
}

// \brief Handles all buffer creation operations.
// Buffer size, memory properties and usage are customizable.
// \returns a populated Buffer struct (Buffer and Buffer memory)
// \throws runtime_error is buffer creation fails
inline BufferBundle create_buffer(const BufferCreationInput &buffer_input) {
  BufferBundle buffer_bundle;
  vk::BufferCreateInfo buffer_info{
      .flags = vk::BufferCreateFlags(),
      .size = buffer_input.size,
      .usage = buffer_input.usage,
      .sharingMode = vk::SharingMode::eExclusive,
  };

  // Error handling with nullptr
  if ((buffer_bundle.buffer = buffer_input.logical_device.createBuffer(
           buffer_info, nullptr)) == nullptr) {
    throw std::runtime_error("failed to create vertex buffer!");
  }

  // Memory requirements
  vk::MemoryRequirements mem_requirements =
      buffer_input.logical_device.getBufferMemoryRequirements(
          buffer_bundle.buffer);

  // Memory allocation
  vk::MemoryAllocateInfo alloc_info{};
  alloc_info.allocationSize = mem_requirements.size;
  alloc_info.memoryTypeIndex = find_memory_type_index(
      buffer_input.physical_device, mem_requirements.memoryTypeBits,
      buffer_input.memory_properties);

  if ((buffer_bundle.buffer_memory = buffer_input.logical_device.allocateMemory(
           alloc_info, nullptr)) == nullptr) {
    throw std::runtime_error("failed to allocate vertex buffer memory!");
  }

  // If offset is non-zero, it is required to be divisible by
  // memRequirements.alignment
  buffer_input.logical_device.bindBufferMemory(buffer_bundle.buffer,
                                               buffer_bundle.buffer_memory, 0);

  return buffer_bundle;
}

inline vk::Result copy_buffer(const BufferBundle &src_buffer,
                              BufferBundle &dst_buffer, vk::DeviceSize size,
                              vk::Queue queue,
                              vk::CommandBuffer command_buffer) {

  command_buffer.reset();

  vk::CommandBufferBeginInfo begin_info{
      .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
  command_buffer.begin(begin_info);

  /*
  * // Provided by VK_VERSION_1_0
  typedef struct VkBufferCopy {
          VkDeviceSize    srcOffset;
          VkDeviceSize    dstOffset;
          VkDeviceSize    size;
  } VkBufferCopy;
  */
  vk::BufferCopy copy_region{.srcOffset = 0, .dstOffset = 0, .size = size};
  command_buffer.copyBuffer(src_buffer.buffer, dst_buffer.buffer, 1,
                            &copy_region);

  command_buffer.end();

  vk::SubmitInfo submit_info{.commandBufferCount = 1,
                             .pCommandBuffers = &command_buffer};

  vk::Result result = queue.submit(1, &submit_info, nullptr);
  queue.waitIdle();
  return result;
}
} // namespace ice
#endif