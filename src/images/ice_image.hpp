#ifndef ICE_IMAGE_HPP
#define ICE_IMAGE_HPP

#include "../config.hpp"
#include <stb_image.h>

namespace ice_image {

struct TextureCreationInput {
  vk::PhysicalDevice physical_device;
  vk::Device logical_device;
  vk::CommandBuffer command_buffer;
  vk::Queue queue;
  vk::DescriptorSetLayout layout;
  vk::DescriptorPool descriptor_pool;
  const char *filename;
};

// VkImage creation struct
struct ImageCreationInput {
  vk::Device logical_device;
  vk::PhysicalDevice physical_device;
  int width, height;
  vk::ImageTiling tiling;
  vk::ImageUsageFlags usage;
  vk::MemoryPropertyFlags memory_properties;
};

// input needed for image layout transitions jobs
struct ImageLayoutTransitionJob {
  vk::CommandBuffer command_buffer;
  vk::Queue queue;
  vk::Image image;
  vk::ImageLayout old_layout, new_layout;
};

// input necessary for copying a buffer to an image
struct BufferImageCopyJob {
  vk::CommandBuffer command_buffer;
  vk::Queue queue;
  vk::Buffer src_buffer;
  vk::Image dst_image;
  int width, height;
};

class Texture {

public:
  Texture(const TextureCreationInput &input);

  void use(vk::CommandBuffer command_buffer,
           vk::PipelineLayout pipeline_layout);

  ~Texture();

private:
  int width, height, channels;
  vk::Device logical_device;
  vk::PhysicalDevice physical_device;
  const char *filename;
  stbi_uc *pixels;

  // Resources
  vk::Image image;
  vk::DeviceMemory image_memory;
  vk::ImageView image_view;
  vk::Sampler sampler;

  // Resource Descriptors
  vk::DescriptorSetLayout layout;
  vk::DescriptorSet descriptor_set;
  vk::DescriptorPool descriptor_pool;

  vk::CommandBuffer command_buffer;
  vk::Queue queue;

  // Load the raw image data from the internally set filepath.
  void load();

  /**
   * Send loaded data to the image. The image must be loaded before calling
   * this function.
   */
  void populate();

  /**
   * Create a view of the texture. The image must be populated before
   * calling this function.
   */
  void make_view();

  // Configure and create a sampler for the texture.
  void make_sampler();

  /**
   * Allocate and write the descriptor set. Currently, this is only being
   * done once. This must be called after the image view and sampler have been
   * made.
   */
  void make_descriptor_set();
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
vk::ImageView make_image_view(vk::Device logical_device, vk::Image image,
                              vk::Format format);
} // namespace ice_image

#endif