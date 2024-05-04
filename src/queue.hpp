#ifndef QUEUE_H
#define QUEUE_H

#include "config.hpp"

namespace ice {

// stores different Message Queue Indices
struct QueueFamilyIndices {
  std::optional<std::uint32_t> graphics_family;
  std::optional<std::uint32_t> present_family;

  bool is_complete() {
    // families supporting drawing and presentation may not overlap, we want
    // both to be supported
    return graphics_family.has_value() && present_family.has_value();
  }
};

inline QueueFamilyIndices
find_queue_families(const vk::PhysicalDevice &p_device,
                    const vk::SurfaceKHR &surface) {
  QueueFamilyIndices indices;

  std::uint32_t queue_family_count = 0;
  std::vector<vk::QueueFamilyProperties> queue_families =
      p_device.getQueueFamilyProperties();

  std::uint32_t i{0};
  for (const auto &queue_family : queue_families) {
    if (queue_family.queueFlags &
        vk::QueueFlagBits::eGraphics) { // support drawing commands
      indices.graphics_family = i;
    }

    auto present_support = p_device.getSurfaceSupportKHR(i, surface);
    if (present_support) {
      indices.present_family = i;
    }

    if (indices.is_complete()) {
      break;
    }
    i++;
  }
  return indices;
}
} // namespace ice

#endif