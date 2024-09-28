#include "commands.hpp"
#include "config.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "../commands.hpp"
#include "../data_buffers.hpp"
#include "../descriptors.hpp"
#include "ice_image.hpp"

namespace ice_image {
vk::Image make_image(const ImageCreationInput &input) {
  const vk::ImageCreateInfo image_info{
      .flags = vk::ImageCreateFlagBits() | input.create_flags,

      .imageType = vk::ImageType::e2D,
      .format = input.format,
      .extent = {input.width, input.height, 1},

      .mipLevels = input.mip_levels,
      .arrayLayers = input.array_count,

      .samples = input.msaa_samples,
      .tiling = input.tiling,
      .usage = input.usage,

      .sharingMode = vk::SharingMode::eExclusive,
      .initialLayout = vk::ImageLayout::eUndefined,
  };

  vk::Image image;
  try {
    image = input.logical_device.createImage(image_info);
  } catch (const vk::SystemError &err) {
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
  std::cout << std::format(
      "Required size:             {}\n"
      "Required memory type bit:  {}\n",
      requirements.size, requirements.memoryTypeBits);
#endif

  const vk::MemoryAllocateInfo allocation{
      .allocationSize = requirements.size,
      .memoryTypeIndex = ice::find_memory_type_index(
          input.physical_device, requirements.memoryTypeBits,
          vk::MemoryPropertyFlagBits::eDeviceLocal)};

  vk::DeviceMemory image_memory;
  try {
    image_memory = input.logical_device.allocateMemory(allocation);
    input.logical_device.bindImageMemory(image, image_memory, 0);
  } catch (const vk::SystemError &err) {
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

  const vk::ImageSubresourceRange access{
      .aspectMask = vk::ImageAspectFlagBits::eColor,
      .baseMipLevel = 0,
      .levelCount = transition_job.mip_levels,
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
      .imageOffset = {.x = 0, .y = 0, .z = 0},
      .imageExtent = {static_cast<uint32_t>(copy_job.width),
                      static_cast<uint32_t>(copy_job.height), 1}};

  const vk::ImageSubresourceLayers access{
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

vk::ImageView make_image_view(vk::Device logical_device, vk::Image image,
                              vk::Format format, vk::ImageAspectFlags aspect,
                              vk::ImageViewType view_type,
                              std::uint32_t array_count,
                              std::uint32_t mip_levels) {
  const vk::ImageViewCreateInfo create_info = {
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
          .levelCount = mip_levels,
          .baseArrayLayer = 0,
          .layerCount = array_count,
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
  for (const vk::Format format : candidates) {
    const vk::FormatProperties properties =
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

void generate_mipmaps(vk::PhysicalDevice physical_device,
                      vk::CommandBuffer command_buffer, vk::Image image,
                      vk::Queue graphics_queue, vk::Format image_format,
                      std::uint32_t tex_width, std::uint32_t tex_height,
                      std::uint32_t mip_levels) {
  // Check if image format support linear blitting
  const vk::FormatProperties format_properties =
      physical_device.getFormatProperties(image_format);

  if (!(format_properties.optimalTilingFeatures &
        vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
    // may introduce a software CPU side blitting rather than run time errors
    throw std::runtime_error(
        "texture image format does not support linear blitting");
  }

  ice::start_job(command_buffer);

  // reused for the several transitions
  vk::ImageMemoryBarrier barrier{
      .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
      .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
      .image = image,
      .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor,
                           .levelCount = 1,
                           .baseArrayLayer = 0,
                           .layerCount = 1}};

  auto mip_width = static_cast<int32_t>(tex_width);
  auto mip_height = static_cast<int32_t>(tex_height);

  for (std::uint32_t i = 1; i < mip_levels; i++) {
    // transition level i - 1 to TRANSFER_SRC_OPTIMAL.
    // This transition will wait for the level to be filled either from
    // previous blit command or from vkCmdCopyBufferToImage.
    // The current blit command will wait on this transition
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;  // eUndefined
    barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

    command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                   vk::PipelineStageFlagBits::eTransfer,
                                   vk::DependencyFlags(), 0, nullptr, 0,
                                   nullptr, 1, &barrier);

    // regions to be used in the blit operation.
    vk::ImageBlit blit{
        .srcSubresource =
            {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .mipLevel = i - 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },

        .dstSubresource = {.aspectMask = vk::ImageAspectFlagBits::eColor,
                           .mipLevel = i,
                           .baseArrayLayer = 0,
                           .layerCount = 1}};
    // The 2 elements of the srcOffsets array determine the 3D region that
    // data will be blitted from . 2D image has a depth of 1.
    blit.srcOffsets[0] = {0, 0, 0};
    blit.srcOffsets[1] = {mip_width, mip_height, 1};

    blit.dstOffsets[0] = {0, 0, 0};
    blit.dstOffsets[1] = {mip_width > 1 ? mip_width / 2 : 1,
                          mip_height > 1 ? mip_height / 2 : 1, 1};

    // record blit command
    // blitting between different levels of the same image. The source mip level
    // was just transitioned to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL and the
    // destination level is still in VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL from
    // createTextureImage. vk::Filter::eLinear to enable interpolation.
    command_buffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image,
                             vk::ImageLayout::eTransferDstOptimal, blit,
                             vk::Filter::eLinear);

    barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    // waits for the current blit command to finish.
    // All sampling operation will wait on this transition to finish.
    command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                   vk::PipelineStageFlagBits::eFragmentShader,
                                   vk::DependencyFlags(), 0, nullptr, 0,
                                   nullptr, 1, &barrier);

    // ensure it can never become 0 especially for non-square images
    if (mip_width > 1) mip_width /= 2;
    if (mip_height > 1) mip_height /= 2;
  }

  // transition the last mip level from TransferDstOptimal to
  // ShaderReadOnlyOptimal.
  barrier.subresourceRange.baseMipLevel = mip_levels - 1;
  barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;  // eUndefined
  barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
  barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
  barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

  command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                 vk::PipelineStageFlagBits::eFragmentShader,
                                 vk::DependencyFlags(), 0, nullptr, 0, nullptr,
                                 1, &barrier);

  ice::end_job(command_buffer, graphics_queue);
}

}  // namespace ice_image
