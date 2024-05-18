#ifndef ICE_TEXTURE_HPP
#define ICE_TEXTURE_HPP

#include "../config.hpp"
#include <stb_image.h>
#include "ice_image.hpp"

namespace ice_image {

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
} // namespace ice_image

#endif