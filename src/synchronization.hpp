#ifndef SYNCHRONIZATION
#define SYNCHRONIZATION

#include "config.hpp"

namespace ice {

inline vk::Semaphore make_semaphore(vk::Device device) {
#ifndef NDEBUG
  std::cout << "Made a Semaphore" << std::endl;
#endif
  vk::SemaphoreCreateInfo semaphoreInfo{.flags = vk::SemaphoreCreateFlags()};

  try {
    return device.createSemaphore(semaphoreInfo);
  } catch (vk::SystemError err) {
#ifndef NDEBUG
    std::cout << "Failed to create semaphore " << std::endl;
#endif
    throw std::runtime_error("Failed to create semaphore ");
  }
}

inline vk::Fence make_fence(vk::Device device) {
#ifndef NDEBUG
  std::cout << "Made a Fence" << std::endl;
#endif
  vk::FenceCreateInfo fence_info = {
      /* Signaled on creation */
      .flags = vk::FenceCreateFlags() | vk::FenceCreateFlagBits::eSignaled};

  try {
    return device.createFence(fence_info);
  } catch (vk::SystemError err) {
#ifndef NDEBUG
    std::cout << "Failed to create Fence " << std::endl;
#endif
    throw std::runtime_error("Failed to create fence ");
  }
}
} // namespace ice
#endif