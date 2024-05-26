#ifndef ICE_IMAGE_HPP
#define ICE_IMAGE_HPP

#include "../config.hpp"
#include <stb_image.h>

namespace ice_image {

// Creation struct for textures and Maps
struct TextureCreationInput {
  vk::PhysicalDevice physical_device;
  vk::Device logical_device;
  vk::CommandBuffer command_buffer;
  vk::Queue queue;
  vk::DescriptorSetLayout layout;
  vk::DescriptorPool descriptor_pool;
  std::vector<const char *> filenames;
};

// VkImage creation struct
struct ImageCreationInput {
  vk::Device logical_device;
  vk::PhysicalDevice physical_device;
  std::uint32_t width, height;
  vk::ImageTiling tiling;
  vk::ImageUsageFlags usage;
  vk::MemoryPropertyFlags memory_properties;
  vk::Format format;
  std::uint32_t array_count{1};
  vk::ImageCreateFlags create_flags;
  std::uint32_t mip_levels{1};
};

// input needed for image layout transitions jobs
struct ImageLayoutTransitionJob {
  vk::CommandBuffer command_buffer;
  vk::Queue queue;
  vk::Image image;
  vk::ImageLayout old_layout, new_layout;
  std::uint32_t array_count{1};
  std::uint32_t mip_levels{1};
};

// input necessary for copying a buffer to an image
struct BufferImageCopyJob {
  vk::CommandBuffer command_buffer;
  vk::Queue queue;
  vk::Buffer src_buffer;
  vk::Image dst_image;
  int width, height;
  std::uint32_t array_count{1};
};

// Make a Vulkan Image
vk::Image make_image(const ImageCreationInput &input);

/**
 * Allocate and bind the backing memory for a Vulkan Image, this memory
 * must be freed upon image destruction.
 */
vk::DeviceMemory make_image_memory(const ImageCreationInput &input,
                                   vk::Image image);

/**
 * Transition the layout of an image.
 * Currently supports:
 * undefined -> transfer_dst_optimal,
 * transfer_dst_optimal -> shader_read_only_optimal,
 */
void transition_image_layout(ImageLayoutTransitionJob transition_job);

/**
 * Copy from a buffer to an image. Image must be in the
 * transfer_dst_optimal layout.
 */
void copy_buffer_to_image(const BufferImageCopyJob &copy_job);

// Create a view of a vulkan image.
vk::ImageView
make_image_view(vk::Device logical_device, vk::Image image, vk::Format format,
                vk::ImageAspectFlags aspect,
                vk::ImageViewType view_type = vk::ImageViewType::e2D,
                std::uint32_t array_count = 1, std::uint32_t mip_levels = 1);

vk::Format find_supported_format(vk::PhysicalDevice physical_device,
                                 const std::vector<vk::Format> &candidates,
                                 vk::ImageTiling tiling,
                                 vk::FormatFeatureFlags features);

/**
 * Generate mipmaps using the GPU. We need a command buffer and a queue to
 * submit the instructions to.
 * It should be submitted to a queue with graphics capability for the Blit
 * command to work.
 * It expects old image layout to be TransferDstOptimal. It will transition to
 * ShaderReadOnlyOptimal when done
 */
void generate_mipmaps(vk::PhysicalDevice physical_device,
                      vk::CommandBuffer command_buffer, vk::Image image,
                      vk::Queue graphics_queue, vk::Format image_format,
                      uint32_t tex_width, uint32_t tex_height,
                      std::uint32_t mip_levels);

} // namespace ice_image

#endif