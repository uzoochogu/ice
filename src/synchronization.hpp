#ifndef SYNCHRONIZATION_HPP
#define SYNCHRONIZATION_HPP

#include "config.hpp"

namespace ice {

inline vk::Semaphore make_semaphore(vk::Device device) {
#ifndef NDEBUG
  std::cout << "Made a Semaphore" << std::endl;
#endif
  const vk::SemaphoreCreateInfo semaphore_info{.flags =
                                                   vk::SemaphoreCreateFlags()};

  try {
    return device.createSemaphore(semaphore_info);
  } catch (const vk::SystemError& err) {
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
  const vk::FenceCreateInfo fence_info = {
      .flags = vk::FenceCreateFlags() | vk::FenceCreateFlagBits::eSignaled};

  try {
    return device.createFence(fence_info);
  } catch (const vk::SystemError& err) {
#ifndef NDEBUG
    std::cout << "Failed to create Fence " << std::endl;
#endif
    throw std::runtime_error("Failed to create fence ");
  }
}
}  // namespace ice

#endif  // SYCHRONIZATION_HPP
