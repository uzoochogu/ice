#include "ice_texture.hpp"
#include "../data_buffers.hpp"
#include "../descriptors.hpp"

namespace ice_image {

Texture::Texture(const TextureCreationInput &input) { load(input); }

void Texture::load(const TextureCreationInput &input) {

  logical_device = input.logical_device;
  physical_device = input.physical_device;
  filename = input.filenames[0];
  command_buffer = input.command_buffer;
  queue = input.queue;
  layout = input.layout;
  descriptor_pool = input.descriptor_pool;

  load();

  ImageCreationInput image_input{.logical_device = logical_device,
                                 .physical_device = physical_device,
                                 .width = static_cast<std::uint32_t>(width),
                                 .height = static_cast<std::uint32_t>(height),
                                 .tiling = vk::ImageTiling::eOptimal,
                                 .usage = vk::ImageUsageFlagBits::eTransferDst |
                                          vk::ImageUsageFlagBits::eSampled,
                                 .memory_properties =
                                     vk::MemoryPropertyFlagBits::eDeviceLocal,
                                 .format = vk::Format::eR8G8B8A8Unorm,
                                 .array_count = 1};

  image = make_image(image_input);
#ifndef NDEBUG
  std::cout << "Finished Creating the VkImage\n\n" << std::endl;
#endif

  image_memory = make_image_memory(image_input, image);

  populate();

  free(pixels);

  make_view();

  make_sampler();

  make_descriptor_set();
#ifndef NDEBUG
  std::cout << "Finished Creating the ice_image::Texture Object\n\n"
            << std::endl;
#endif
}

Texture::~Texture() {
  logical_device.freeMemory(image_memory);
  logical_device.destroyImage(image);
  logical_device.destroyImageView(image_view);
  logical_device.destroySampler(sampler);
}

void Texture::load() {
#ifndef NDEBUG
  std::cout << "\nLoading Textures.....\n";
#endif
  pixels = stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha);
  if (!pixels) {
    std::cout << std::format("Unable to load: {}", filename) << std::endl;
  }
}

void Texture::populate() {

  // First create a CPU-visible buffer...
  ice::BufferCreationInput input{
      .size = static_cast<size_t>(width * height * 4),
      .usage = vk::BufferUsageFlagBits::eTransferSrc,

      .memory_properties = vk::MemoryPropertyFlagBits::eHostCoherent |
                           vk::MemoryPropertyFlagBits::eHostVisible,

      .logical_device = logical_device,
      .physical_device = physical_device,

  };

  ice::BufferBundle staging_buffer = create_buffer(input);

  // fill it,
  void *write_location =
      logical_device.mapMemory(staging_buffer.buffer_memory, 0, input.size);
  memcpy(write_location, pixels, input.size);
  logical_device.unmapMemory(staging_buffer.buffer_memory);

  // then transfer it to image memory
  ImageLayoutTransitionJob transition_job{
      .command_buffer = command_buffer,
      .queue = queue,
      .image = image,
      .old_layout = vk::ImageLayout::eUndefined,
      .new_layout = vk::ImageLayout::eTransferDstOptimal};

  transition_image_layout(transition_job);

  BufferImageCopyJob copy_job{.command_buffer = command_buffer,
                              .queue = queue,
                              .src_buffer = staging_buffer.buffer,
                              .dst_image = image,
                              .width = width,
                              .height = height};

  copy_buffer_to_image(copy_job);

  transition_job.old_layout = vk::ImageLayout::eTransferDstOptimal;
  transition_job.new_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
  transition_image_layout(transition_job);

  // Now the staging buffer can be destroyed
  logical_device.freeMemory(staging_buffer.buffer_memory);
  logical_device.destroyBuffer(staging_buffer.buffer);
}

void Texture::make_view() {
  image_view = make_image_view(
      logical_device, image, vk::Format::eR8G8B8A8Unorm,
      vk::ImageAspectFlagBits::eColor, vk::ImageViewType::e2D, 1);
}

void Texture::make_sampler() {
  vk::SamplerCreateInfo sampler_info{
      .flags = vk::SamplerCreateFlags(),
      .magFilter = vk::Filter::eLinear,
      .minFilter = vk::Filter::eNearest,
      .mipmapMode = vk::SamplerMipmapMode::eLinear,
      .addressModeU = vk::SamplerAddressMode::eRepeat,
      .addressModeV = vk::SamplerAddressMode::eRepeat,
      .addressModeW = vk::SamplerAddressMode::eRepeat,
      .anisotropyEnable = false,
      .maxAnisotropy = 1.0f,

      .compareEnable = false,
      .compareOp = vk::CompareOp::eAlways,
      .borderColor = vk::BorderColor::eIntOpaqueBlack,
      .unnormalizedCoordinates = false};

  try {
    sampler = logical_device.createSampler(sampler_info);
  } catch (vk::SystemError err) {
    std::cout << "Failed to make sampler." << std::endl;
  }
#ifndef NDEBUG
  std::cout << "Finished Creating the sampler\n\n" << std::endl;
#endif
}

void Texture::make_descriptor_set() {

  descriptor_set =
      ice::allocate_descriptor_sets(logical_device, descriptor_pool, layout);

  vk::DescriptorImageInfo image_descriptor{
      .sampler = sampler,
      .imageView = image_view,

      .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
  };

  vk::WriteDescriptorSet descriptor_write{
      .dstSet = descriptor_set,
      .dstBinding = 0,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = vk::DescriptorType::eCombinedImageSampler,
      .pImageInfo = &image_descriptor};

  logical_device.updateDescriptorSets(descriptor_write, nullptr);
}

void Texture::use(vk::CommandBuffer command_buffer,
                  vk::PipelineLayout pipelineLayout) {
  command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                    pipelineLayout, 1, descriptor_set, nullptr);
}

} // namespace ice_image
