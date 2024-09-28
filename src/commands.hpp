#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include "config.hpp"
#include "queue.hpp"
#include "swapchain.hpp"

namespace ice {
struct CommandBufferReq {
  vk::Device device;
  vk::CommandPool command_pool;
  std::vector<SwapChainFrame>& frames;
};

inline vk::CommandPool make_command_pool(vk::Device device,
                                         vk::PhysicalDevice physical_device,
                                         vk::SurfaceKHR surface) {
  const QueueFamilyIndices family_indices =
      find_queue_families(physical_device, surface);

  const vk::CommandPoolCreateInfo pool_info = {
      /* All command buffers can be reset individually */
      .flags = vk::CommandPoolCreateFlags() |
               vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
      .queueFamilyIndex = family_indices.graphics_family.value_or(0)};
  try {
    return device.createCommandPool(pool_info);
  } catch (const vk::SystemError& err) {
    throw std::runtime_error("Failed to create command Pool");
  }
}

inline vk::CommandBuffer make_command_buffer(
    const CommandBufferReq& buffer_req) {
  const vk::CommandBufferAllocateInfo alloc_info{
      .commandPool = buffer_req.command_pool,
      .level = vk::CommandBufferLevel::ePrimary,
      .commandBufferCount = 1};
  // make main command buffer for engine
  try {
    return buffer_req.device.allocateCommandBuffers(alloc_info)[0];
  } catch (const vk::SystemError& err) {
    throw std::runtime_error("Failed to allocate main command buffer ");
  }
}

inline void make_frame_command_buffers(const CommandBufferReq& buffer_req) {
  const vk::CommandBufferAllocateInfo alloc_info{
      .commandPool = buffer_req.command_pool,
      .level = vk::CommandBufferLevel::ePrimary,
      .commandBufferCount = 1};

  // make command buffer for each swapchain frame
  for (int i = 0; i < buffer_req.frames.size(); ++i) {
    try {
      buffer_req.frames[i].command_buffer =
          buffer_req.device.allocateCommandBuffers(
              alloc_info)[0];  // we are creating only one
    } catch (const vk::SystemError& err) {
      throw std::runtime_error(
          std::format("Failed to allocate command buffer for frame {}", i));
    }
  }
}

inline void make_imgui_command_buffers(const CommandBufferReq& buffer_req) {
  const vk::CommandBufferAllocateInfo alloc_info{
      .commandPool = buffer_req.command_pool,
      .level = vk::CommandBufferLevel::ePrimary,
      .commandBufferCount = 1};

  for (size_t i = 0; i < buffer_req.frames.size(); ++i) {
    try {
      buffer_req.frames[i].imgui_command_buffer =
          buffer_req.device.allocateCommandBuffers(
              alloc_info)[0];  // one per frame
    } catch (const vk::SystemError& err) {
      throw std::runtime_error(std::format(
          "Failed to allocate imgui command buffer for frame {}", i));
    }
  }
}

// Begin recording a command buffer intended for a single submit.
inline void start_job(vk::CommandBuffer command_buffer) {
  command_buffer.reset();

  const vk::CommandBufferBeginInfo begin_info{
      .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
  command_buffer.begin(begin_info);
}

// Finish recording a command buffer and submit it.
inline void end_job(vk::CommandBuffer command_buffer,
                    vk::Queue submission_queue) {
  command_buffer.end();

  const vk::SubmitInfo submit_info{.commandBufferCount = 1,
                                   .pCommandBuffers = &command_buffer};
  auto result = submission_queue.submit(1, &submit_info, nullptr);
  submission_queue.waitIdle();
}
}  // namespace ice

#endif  // COMMANDS_HPP
