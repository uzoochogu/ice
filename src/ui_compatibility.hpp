#ifndef UI_COMPATIBILITY_HPP
#define UI_COMPATIBILITY_HPP

#include "vulkan_ice.hpp"

namespace ui_compatibility {
// Sets the MSAA samples for the given backend, returns the new sample count
inline int set_msaa_samples(ice::VulkanIce& backend, int samples) {
  vk::SampleCountFlagBits new_msaa_samples;
  switch (samples) {
    case 0:
      new_msaa_samples = vk::SampleCountFlagBits::e1;
#ifndef NDEBUG
      std::cout << "Warning: Not ideal since we are resolving "
                   "attachments regardless\n";
#endif
      break;
    case 1:
      new_msaa_samples = vk::SampleCountFlagBits::e2;
      break;
    case 2:
      new_msaa_samples = vk::SampleCountFlagBits::e4;
      break;
    case 3:
      new_msaa_samples = vk::SampleCountFlagBits::e8;
      break;
    case 4:
      new_msaa_samples = vk::SampleCountFlagBits::e16;
      break;
    default:
      new_msaa_samples = vk::SampleCountFlagBits::e8;
      samples = 3;
#ifndef NDEBUG
      std::cout << "Warning: Invalid sample count selected. Defaulting to 8x."
                << std::endl;
#endif
      break;
  }

  vk::SampleCountFlagBits max_msaa_samples = backend.get_max_sample_count();
  if (new_msaa_samples > max_msaa_samples) {
    new_msaa_samples = max_msaa_samples;
    samples = 3;
#ifndef NDEBUG
    std::cout << std::format(
        "Warning: Max supported sample count is {0}. Defaulting to "
        "{0}.\n",
        vk::to_string(max_msaa_samples));
#endif
  }
  backend.set_msaa_samples(new_msaa_samples);
  return samples;
}

// Sets the Culling Mode for given backend, returns the new culling mode value
inline int set_cull_mode(ice::VulkanIce& backend, int cull_current) {
  vk::CullModeFlagBits cull_mode;
  switch (cull_current) {
    case 0:
      cull_mode = vk::CullModeFlagBits::eNone;
      break;
    case 1:
      cull_mode = vk::CullModeFlagBits::eFront;
      break;
    case 2:
      cull_mode = vk::CullModeFlagBits::eBack;
      break;
    case 3:
      cull_mode = vk::CullModeFlagBits::eFrontAndBack;
      break;
    default:
      cull_mode = vk::CullModeFlagBits::eBack;
      cull_current = 2;
#ifndef NDEBUG
      std::cout << "Warning: Invalid cull mode selected. Defaulting to Back."
                << std::endl;
#endif
      break;
  }
  backend.set_cull_mode(cull_mode);
  return cull_current;
}
}  // namespace ui_compatibility

#endif  // UI_COMPATIBILITY_HPP
