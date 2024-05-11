#define STB_IMAGE_IMPLEMENTATION
#include "ice_image.hpp"
#include "../commands.hpp"
#include "../data_buffers.hpp"
#include "../descriptors.hpp"

namespace ice_image {

Texture::Texture(const TextureCreationInput &input) {

  logical_device = input.logical_device;
  physical_device = input.physical_device;
  filename = input.filename;
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
                                 .format = vk::Format::eR8G8B8A8Unorm};

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
  image_view =
      make_image_view(logical_device, image, vk::Format::eR8G8B8A8Unorm,
                      vk::ImageAspectFlagBits::eColor);
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

vk::Image ice_image::make_image(const ImageCreationInput &input) {
  vk::ImageCreateInfo image_info{
      .flags = vk::ImageCreateFlagBits(),

      .imageType = vk::ImageType::e2D,
      .format = input.format,
      .extent = vk::Extent3D(input.width, input.height, 1),

      .mipLevels = 1,
      .arrayLayers = 1,

      .samples = vk::SampleCountFlagBits::e1,
      .tiling = input.tiling,
      .usage = input.usage,

      .sharingMode = vk::SharingMode::eExclusive,
      .initialLayout = vk::ImageLayout::eUndefined,
  };

  vk::Image image;
  try {
    image = input.logical_device.createImage(image_info);
  } catch (vk::SystemError err) {
    std::cout << "Unable to make image";
  }
  return image;
}

vk::DeviceMemory ice_image::make_image_memory(const ImageCreationInput &input,
                                              vk::Image image) {
  vk::MemoryRequirements requirements =
      input.logical_device.getImageMemoryRequirements(image);

#ifndef NDEBUG
  std::cout << "\nImage memory Requirements\n";
  std::cout << std::format("Required size:             {}\n"
                           "Required memory type bit:  {}\n",
                           requirements.size, requirements.memoryTypeBits);
#endif

  vk::MemoryAllocateInfo allocation{
      .allocationSize = requirements.size,
      .memoryTypeIndex = ice::find_memory_type_index(
          input.physical_device, requirements.memoryTypeBits,
          vk::MemoryPropertyFlagBits::eDeviceLocal)};

  vk::DeviceMemory image_memory;
  try {
    image_memory = input.logical_device.allocateMemory(allocation);
    input.logical_device.bindImageMemory(image, image_memory, 0);

  } catch (vk::SystemError err) {
    std::cout << "Unable to allocate memory for image";
    throw err;
  }

#ifndef NDEBUG
  std::cout << "\nFinished creating image memory" << std::endl;
#endif
  return image_memory;
}

void ice_image::transition_image_layout(
    ImageLayoutTransitionJob transition_job) {

  ice::start_job(transition_job.command_buffer);

  vk::ImageSubresourceRange access{.aspectMask =
                                       vk::ImageAspectFlagBits::eColor,
                                   .baseMipLevel = 0,
                                   .levelCount = 1,
                                   .baseArrayLayer = 0,
                                   .layerCount = 1};

  vk::ImageMemoryBarrier barrier{.oldLayout = transition_job.old_layout,
                                 .newLayout = transition_job.new_layout,
                                 .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                 .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                 .image = transition_job.image,
                                 .subresourceRange = access};

  vk::PipelineStageFlags source_stage, destination_stage;

  if (transition_job.old_layout == vk::ImageLayout::eUndefined &&
      transition_job.new_layout == vk::ImageLayout::eTransferDstOptimal) {

    barrier.srcAccessMask = vk::AccessFlagBits::eNoneKHR;
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

    source_stage = vk::PipelineStageFlagBits::eTopOfPipe;
    destination_stage = vk::PipelineStageFlagBits::eTransfer;
  } else {

    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    source_stage = vk::PipelineStageFlagBits::eTransfer;
    destination_stage = vk::PipelineStageFlagBits::eFragmentShader;
  }

  transition_job.command_buffer.pipelineBarrier(source_stage, destination_stage,
                                                vk::DependencyFlags(), nullptr,
                                                nullptr, barrier);

  ice::end_job(transition_job.command_buffer, transition_job.queue);
}

void ice_image::copy_buffer_to_image(const BufferImageCopyJob &copy_job) {

  ice::start_job(copy_job.command_buffer);

  vk::BufferImageCopy copy{
      .bufferOffset = 0,
      .bufferRowLength = 0,
      .bufferImageHeight = 0,
      .imageOffset = vk::Offset3D(0, 0, 0),
      .imageExtent = vk::Extent3D(copy_job.width, copy_job.height, 1)};

  vk::ImageSubresourceLayers access{
      .aspectMask = vk::ImageAspectFlagBits::eColor,
      .mipLevel = 0,
      .baseArrayLayer = 0,
      .layerCount = 1,
  };

  copy.imageSubresource = access;

  copy_job.command_buffer.copyBufferToImage(
      copy_job.src_buffer, copy_job.dst_image,
      vk::ImageLayout::eTransferDstOptimal, copy);

  ice::end_job(copy_job.command_buffer, copy_job.queue);
}

vk::ImageView ice_image::make_image_view(vk::Device logical_device,
                                         vk::Image image, vk::Format format,
                                         vk::ImageAspectFlags aspect) {
  vk::ImageViewCreateInfo create_info = {
      .image = image,
      .viewType = vk::ImageViewType::e2D,
      .format = format,
      .components = {.r = vk::ComponentSwizzle::eIdentity,
                     .g = vk::ComponentSwizzle::eIdentity,
                     .b = vk::ComponentSwizzle::eIdentity,
                     .a = vk::ComponentSwizzle::eIdentity},
      .subresourceRange = {
          .aspectMask = aspect,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1,
      }};

  return logical_device.createImageView(create_info);
}

// utils

// @brief Finds supported format given some candidates and features to look for.
// @exception throws a runtime error if no format is found
vk::Format find_supported_format(vk::PhysicalDevice physical_device,
                                 const std::vector<vk::Format> &candidates,
                                 vk::ImageTiling tiling,
                                 vk::FormatFeatureFlags features) {
  for (vk::Format format : candidates) {

    vk::FormatProperties properties =
        physical_device.getFormatProperties(format);

    if (tiling == vk::ImageTiling::eLinear &&
        (properties.linearTilingFeatures & features) == features) {
      return format;
    }

    if (tiling == vk::ImageTiling::eOptimal &&
        (properties.optimalTilingFeatures & features) == features) {
      return format;
    }
  }
  throw std::runtime_error("Unable to find suitable format");
}

} // namespace ice_image
