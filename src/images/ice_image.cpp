#include "config.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "../commands.hpp"
#include "../data_buffers.hpp"
#include "../descriptors.hpp"
#include "ice_image.hpp"


namespace ice_image {
vk::Image make_image(const ImageCreationInput &input) {
  vk::ImageCreateInfo image_info{
      .flags = vk::ImageCreateFlagBits() | input.create_flags,

      .imageType = vk::ImageType::e2D,
      .format = input.format,
      .extent = vk::Extent3D(input.width, input.height, 1),

      .mipLevels = 1,
      .arrayLayers = input.array_count,

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

vk::DeviceMemory make_image_memory(const ImageCreationInput &input,
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

void transition_image_layout(ImageLayoutTransitionJob transition_job) {

  ice::start_job(transition_job.command_buffer);

  vk::ImageSubresourceRange access{.aspectMask =
                                       vk::ImageAspectFlagBits::eColor,
                                   .baseMipLevel = 0,
                                   .levelCount = 1,
                                   .baseArrayLayer = 0,
                                   .layerCount = transition_job.array_count};

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

void copy_buffer_to_image(const BufferImageCopyJob &copy_job) {

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
      .layerCount = copy_job.array_count,
  };

  copy.imageSubresource = access;

  copy_job.command_buffer.copyBufferToImage(
      copy_job.src_buffer, copy_job.dst_image,
      vk::ImageLayout::eTransferDstOptimal, copy);

  ice::end_job(copy_job.command_buffer, copy_job.queue);
}

vk::ImageView
make_image_view(vk::Device logical_device, vk::Image image, vk::Format format,
                vk::ImageAspectFlags aspect,
                vk::ImageViewType view_type,
                std::uint32_t array_count) {
  vk::ImageViewCreateInfo create_info = {
      .image = image,
      .viewType = view_type,
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
          .layerCount = array_count,
      }};

  return logical_device.createImageView(create_info);
}

// utils

// @brief Finds supported format given some candidates and features to look for.
// @exception throws a runtime error if no format is found
vk::Format
find_supported_format(vk::PhysicalDevice physical_device,
                      const std::vector<vk::Format> &candidates,
                      vk::ImageTiling tiling, vk::FormatFeatureFlags features) {
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
