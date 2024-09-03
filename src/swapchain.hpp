#ifndef SWAPCHAIN_HPP
#define SWAPCHAIN_HPP

#include "./images/ice_image.hpp"
#include "camera.hpp"
#include "config.hpp"
#include "data_buffers.hpp"
#include "queue.hpp"
#include "windowing.hpp"

namespace ice {

// @brief Bundles everything related to a swapchain frame: image, image view,
// frame buffer, command buffer, per-frame descriptors like UBO and model
// transforms and synchronization objects
struct SwapChainFrame {
  vk::PhysicalDevice physical_device;
  vk::Device logical_device;

  // swapchain essentials
  vk::Image image;
  vk::ImageView image_view;
  std::unordered_map<PipelineType, vk::Framebuffer> framebuffer;
  vk::Framebuffer imgui_framebuffer;

  // depth resources
  vk::Image depth_buffer;
  vk::DeviceMemory depth_buffer_memory;
  vk::ImageView depth_buffer_view;
  vk::Format depth_format;

  // color resources
  vk::SampleCountFlagBits msaa_samples;
  vk::Image color_buffer;
  vk::DeviceMemory color_buffer_memory;
  vk::ImageView color_buffer_view;

  vk::Extent2D extent;

  vk::CommandBuffer command_buffer;
  vk::CommandBuffer imgui_command_buffer;

  // sync objects
  vk::Semaphore image_available, render_finished;
  vk::Fence in_flight_fence;

  // frame resources
  CameraMatrices camera_matrix_data;
  BufferBundle camera_matrix_buffer;
  void *camera_matrix_write_location;

  vk::Format color_format;

  CameraVectors camera_vector_data;
  BufferBundle camera_vector_buffer;
  void *camera_vector_write_location;

  std::vector<glm::mat4> model_transforms;
  BufferBundle model_buffer;
  void *model_buffer_write_location;

  // Resource Descriptors
  vk::DescriptorBufferInfo camera_vector_descriptor_info,
      camera_matrix_descriptor_info;
  vk::DescriptorBufferInfo ssbo_descriptor_info;
  std::unordered_map<PipelineType, vk::DescriptorSet> descriptor_sets;

  // Write Operations
  std::vector<vk::WriteDescriptorSet> write_ops;

  void make_descriptor_resources();

  void make_depth_resources();

  void make_color_resources();

  void record_write_operations();

  void write_descriptor_set();

  void destroy(vk::CommandPool imgui_command_pool);
};

// Bundles handles that would be created  with swapchain:
// swapchain, SwapchainFrame struct , format and extents.
struct SwapChainBundle {
  vk::SwapchainKHR swapchain;
  std::vector<SwapChainFrame> frames;
  vk::Format format;
};

// Stores swapchain support details: capabilities, formats and
// present modes
struct SwapChainSupportDetails {
  vk::SurfaceCapabilitiesKHR capabilities;
  std::vector<vk::SurfaceFormatKHR> formats;
  std::vector<vk::PresentModeKHR> present_modes;
};

inline SwapChainSupportDetails
query_swapchain_support(const vk::PhysicalDevice &physical_device,
                        const vk::SurfaceKHR &surface) {
  SwapChainSupportDetails details;
  details.capabilities = physical_device.getSurfaceCapabilitiesKHR(surface);
  details.formats = physical_device.getSurfaceFormatsKHR(surface);
  details.present_modes = physical_device.getSurfacePresentModesKHR(surface);

  return details;
}

// choosers
inline vk::SurfaceFormatKHR choose_swap_surface_format(
    const std::vector<vk::SurfaceFormatKHR> &available_formats) {

  for (const auto &available_format : available_formats) {
    if (available_format.format == vk::Format::eB8G8R8A8Srgb &&
        available_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
      return available_format;
    }
  }

  // Just choose the first if the most preferred is not available
  return available_formats[0];
}

inline vk::Extent2D
choose_swap_extent(std::uint32_t width, std::uint32_t height,
                   const vk::SurfaceCapabilitiesKHR &capabilities) {
  // Some window managers set the currentExtent to maximum value of uint32_t
  // as a special case(else block)
  if (capabilities.currentExtent.width !=
      std::numeric_limits<std::uint32_t>::max()) {
    return capabilities.currentExtent;
  } else { // Pick resolution that best matches window within minImageExtent
           // and maxImageExtent in pixels

    vk::Extent2D actual_extent = {static_cast<uint32_t>(width),
                                  static_cast<uint32_t>(height)};

    // Clamps values
    actual_extent.width =
        std::clamp(actual_extent.width, capabilities.minImageExtent.width,
                   capabilities.maxImageExtent.width);
    actual_extent.height =
        std::clamp(actual_extent.height, capabilities.minImageExtent.height,
                   capabilities.maxImageExtent.height);

    return actual_extent;
  }
}

inline vk::PresentModeKHR
choose_swap_present_mode(const std::vector<vk::PresentModeKHR> &present_modes) {

  for (const auto &present_mode : present_modes) {

    if (present_mode ==
        vk::PresentModeKHR::eMailbox) { // aka Triple Buffering, low latency
      return present_mode;
    }
  }

  // Guaranteed, energy efficient
  return vk::PresentModeKHR::eFifo;
}

inline SwapChainBundle create_swapchain_bundle(
    vk::PhysicalDevice physical_device, vk::Device logical_device,
    vk::SurfaceKHR surface, int width, int height,
    vk::SwapchainKHR *old_swapchain = nullptr,
    vk::SampleCountFlagBits msaa_samples = vk::SampleCountFlagBits::e1) {

  // Get Swapchain support info
  SwapChainSupportDetails swapchain_support =
      query_swapchain_support(physical_device, surface);

  // Choose surface formats
  vk::SurfaceFormatKHR surface_format =
      choose_swap_surface_format(swapchain_support.formats);

  // Choose presentation mode
  vk::PresentModeKHR present_mode =
      choose_swap_present_mode(swapchain_support.present_modes);

  // Get Screen Extents (in pixels)
  vk::Extent2D extent =
      choose_swap_extent(width, height, swapchain_support.capabilities);

  std::uint32_t image_count =
      std::min(swapchain_support.capabilities.maxImageCount,
               swapchain_support.capabilities.minImageCount + 1);

  vk::SwapchainCreateInfoKHR swap_info{
      .surface = surface,
      .minImageCount = image_count,
      .imageFormat = surface_format.format,
      .imageColorSpace = surface_format.colorSpace,
      .imageExtent = extent,
      .imageArrayLayers = 1,
      .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
      .preTransform = swapchain_support.capabilities.currentTransform,
      .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
      .presentMode = present_mode,
      .clipped = vk::True,
      .oldSwapchain = (old_swapchain == nullptr) ? vk::SwapchainKHR(nullptr)
                                                 : *old_swapchain,
  };

  QueueFamilyIndices indices = find_queue_families(physical_device, surface);
  std::uint32_t queue_family_indices[] = {indices.graphics_family.value(),
                                          indices.present_family.value()};

  if (indices.graphics_family != indices.present_family) {
    swap_info.imageSharingMode = vk::SharingMode::eConcurrent;
    swap_info.queueFamilyIndexCount = 2;
    swap_info.pQueueFamilyIndices = queue_family_indices;
  } else {
    swap_info.imageSharingMode = vk::SharingMode::eExclusive;
  }

  SwapChainBundle bundle{};
  try {
    bundle.swapchain = logical_device.createSwapchainKHR(swap_info);
  } catch (const vk::SystemError &e) {
    throw std::runtime_error("failed to create swapchain");
    std::cerr << e.what() << '\n';
  }

  std::vector<vk::Image> images =
      logical_device.getSwapchainImagesKHR(bundle.swapchain);
  bundle.frames.resize(images.size());
  for (size_t i{0}; i < images.size(); i++) {
    bundle.frames[i].physical_device = physical_device;
    bundle.frames[i].logical_device = logical_device;

    bundle.frames[i].extent = extent;
    bundle.frames[i].image = images[i];
    bundle.frames[i].image_view = ice_image::make_image_view(
        logical_device, images[i], surface_format.format,
        vk::ImageAspectFlagBits::eColor);

    bundle.frames[i].color_format = surface_format.format;

    bundle.frames[i].msaa_samples = msaa_samples;

    bundle.frames[i].make_depth_resources();
    bundle.frames[i].make_color_resources();
  }

  bundle.format = surface_format.format;

  return bundle;
}
} // namespace ice

#endif
