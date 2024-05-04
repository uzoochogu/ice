#ifndef SWAPCHAIN_HPP
#define SWAPCHAIN_HPP

#include "config.hpp"
#include "data_buffers.hpp"
#include "queue.hpp"
#include "ubo.hpp"
#include "windowing.hpp"

namespace ice {

// @brief Bundles everything related to a swapchain frame: image, image view,
// frame buffer, command buffer and synchronization objects
struct SwapChainFrame {
  // swapchain essentials
  vk::Image image;
  vk::ImageView image_view;
  vk::Framebuffer framebuffer;

  vk::CommandBuffer command_buffer;

  // sync objects
  vk::Semaphore image_available, render_finished;
  vk::Fence in_flight_fence;

  // frame resources
  UBO camera_data;
  BufferBundle camera_data_buffer;
  void *camera_data_write_location;
  std::vector<glm::mat4> model_transforms;
  BufferBundle model_buffer;
  void *model_buffer_write_location;

  // Resource Descriptors
  vk::DescriptorBufferInfo uniform_buffer_descriptor_info;
  vk::DescriptorBufferInfo model_buffer_descriptor_info;
  vk::DescriptorSet descriptor_set;

  void make_ubo_resources(vk::Device logical_device,
                          vk::PhysicalDevice physical_device) {

    BufferCreationInput input{
        .size = sizeof(UBO),
        .usage = vk::BufferUsageFlagBits::eUniformBuffer,
        .memory_properties = vk::MemoryPropertyFlagBits::eHostVisible |
                             vk::MemoryPropertyFlagBits::eHostCoherent,
        .logical_device = logical_device,

        .physical_device = physical_device,
    };

    camera_data_buffer = createBuffer(input);

    camera_data_write_location = logical_device.mapMemory(
        camera_data_buffer.buffer_memory, 0, sizeof(UBO));

    // model data
    constexpr const std::uint32_t INSTANCES = 1024;
    input.size = INSTANCES * sizeof(glm::mat4);
    input.usage = vk::BufferUsageFlagBits::eStorageBuffer;
    model_buffer = createBuffer(input);

    model_buffer_write_location = logical_device.mapMemory(
        model_buffer.buffer_memory, 0, INSTANCES * sizeof(glm::mat4));

    model_transforms.reserve(INSTANCES);
    for (std::uint32_t i = 0; i < INSTANCES; ++i) {
      model_transforms.push_back(glm::mat4(1.0f));
    }

    /*
    typedef struct VkDescriptorBufferInfo {
            VkBuffer        buffer;
            VkDeviceSize    offset;
            VkDeviceSize    range;
    } VkDescriptorBufferInfo;
    */
    uniform_buffer_descriptor_info = {
        .buffer = camera_data_buffer.buffer, .offset = 0, .range = sizeof(UBO)};

    model_buffer_descriptor_info = {.buffer = model_buffer.buffer,
                                    .offset = 0,
                                    .range = INSTANCES * sizeof(glm::mat4)};
  }

  void write_descriptor_set(vk::Device device) {

    vk::WriteDescriptorSet write_info{
        .dstSet = descriptor_set,
        .dstBinding = 0,
        .dstArrayElement =
            0, // byte offset within binding for inline uniform blocks
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .pBufferInfo = &uniform_buffer_descriptor_info};
    /*
    typedef struct VkWriteDescriptorSet {
            VkStructureType                  sType;
            const void* pNext;
            VkDescriptorSet                  dstSet;
            uint32_t                         dstBinding;
            uint32_t                         dstArrayElement;
            uint32_t                         descriptorCount;
            VkDescriptorType                 descriptorType;
            const VkDescriptorImageInfo* pImageInfo;
            const VkDescriptorBufferInfo* pBufferInfo;
            const VkBufferView* pTexelBufferView;
    } VkWriteDescriptorSet;
    */
    device.updateDescriptorSets(write_info, nullptr);

    // transforms write info
    write_info.dstBinding = 1,
    write_info.descriptorType = vk::DescriptorType::eStorageBuffer;
    write_info.pBufferInfo = &model_buffer_descriptor_info;

    device.updateDescriptorSets(write_info, nullptr);
  }
};

// @brief Bundles handles that would be created  with swapchain:
// swapchain, SwapchainFrame struct , format and extents.
struct SwapChainBundle {
  vk::SwapchainKHR swapchain;
  std::vector<SwapChainFrame> frames;
  vk::Format format;
  vk::Extent2D extent;
};

// @brief stores swapchain support details: capabilities, formats and
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

  // Just choose the first if the most preferred is not availble
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
        vk::PresentModeKHR::eMailbox) { // aka Tripple Buffering, low latency
      return present_mode;
    }
  }

  // Guaranteed, energy efficient
  return vk::PresentModeKHR::eFifo;
}

inline SwapChainBundle
create_swapchain_bundle(vk::PhysicalDevice physical_device,
                        vk::Device logical_device, vk::SurfaceKHR surface,
                        int width, int height,
                        vk::SwapchainKHR *old_swapchain = nullptr) {

  // Get Swapchain suport info
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

    // image view
    vk::ImageViewCreateInfo view_info{
        .image = images[i],
        .viewType = vk::ImageViewType::e2D,
        .format = surface_format.format,
        .components = {.r = vk::ComponentSwizzle::eIdentity,
                       .g = vk::ComponentSwizzle::eIdentity,
                       .b = vk::ComponentSwizzle::eIdentity,
                       .a = vk::ComponentSwizzle::eIdentity},
        .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor,
                             .baseMipLevel = 0,
                             .levelCount = 1,
                             .baseArrayLayer = 0,
                             .layerCount = 1}

    };

    bundle.frames[i].image = images[i];
    bundle.frames[i].image_view = logical_device.createImageView(view_info);
  }
  bundle.format = surface_format.format;
  bundle.extent = extent;

  return bundle;
}
} // namespace ice

#endif