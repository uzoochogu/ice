/* #ifndef TEXTURING
#define TEXTURING

#include "config.hpp"
#define STB_IMAGE_IMPLEMENTATION // needed to include definitions (function
                                 // bodies)
#include <stb_image.h>

namespace ice {

inline VkCommandBuffer beginSingleTimeCommands();
inline void endSingleTimeCommands(VkCommandBuffer);

inline void generateMipmaps(VkPhysicalDevice physicalDevice, VkImage image,
                            VkFormat imageFormat, int32_t texWidth,
                            int32_t texHeight, std::uint32_t mipLevels) {

  // Check if image format support linear blitting
  VkFormatProperties formatProperties;
  vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat,
                                      &formatProperties);

  // Fields are:
  // linearTilingFeatures, optimalTilingFeatures and bufferFeatures that each
  // describe how the format can be used depending on the way it is used.
  // We create a texture image with the optimal tiling format, so can
  // check this feature against the SAMPLED_IMAGE_FILTER_LINEAR_BIT.
  if (!(formatProperties.optimalTilingFeatures &
        VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
    throw std::runtime_error(
        "texture image format does not support linear blitting");
  }

  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  // We'll reuse this for the several transitions
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.image = image;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.subresourceRange.levelCount = 1;

  int32_t mipWidth = texWidth;
  int32_t mipHeight = texHeight;

  // will record the VkCmdBlitImage commands, starts from 1
  for (std::uint32_t i = 1; i < mipLevels; i++) {
    // These fields will change for each transition
    // First we transition level i - 1 to TRANSFER_SRC_OPTIMAL.
    // This transition will wait for the level to be filled either from
    // previous blit command or from vkCmdCopyBufferToImage.
    // The current blit command will wait on this transition
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &barrier);

    // Next specify the regions to be used in the blit operation. The source
    // mip level is i-1 and the destination mip level is i
    VkImageBlit blit{};
    // The 2 elements of the srcOffsets array determine the 3D region that
    // data will be blitted from .
    blit.srcOffsets[0] = {0, 0, 0};
    blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel = i - 1;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = 1;
    // dstoffsets determines the region that data will be blitted to.
    blit.dstOffsets[0] = {0, 0, 0};
    // The X and Y dimensions of this dstOffsets[1] is divided by two since
    // each mip level is half the size of the previous level. Z dimension of
    // srcOffsets[1] and dstOffsets[1] must be 1, since a 2D image has a depth
    // of 1.
    blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1,
                          mipHeight > 1 ? mipHeight / 2 : 1, 1};
    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel = i;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = 1;

    // We record the blit command
    // The image (textureImage) is used for both the srcImage and dstImage
    // params because we're blitting between different levels of the same
    // image. The source mip level was just transitioned to
    // VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL and the destination level is still
    // in VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL from createTextureImage.
    //
    // Note: Beware if you are using a dedicated transfer queue vkCmdBlitImage
    // must be submitted to a queue with graphics capability.
    //
    // Last parameter allows us to specify a VkFilter to use in the blit. Same
    // options as when making VkSampler. We use VK_FILTER_LINEAR to enable
    // interpolation.
    vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                   VK_FILTER_LINEAR);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    // This barrier transitions mip level i- - 1 to
    // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL. This transition waits for the
    // current blit command to finish. All sampling operation will wait on
    // this transition to finish.
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
                         0, nullptr, 1, &barrier);

    // Divide current mip dimensions by 2 at end of loop
    // ensure it can never become 0 thus handling cases where the image is not
    // square since one of the mip dimensionswould reach 1 before the other.
    // When this happens, that dimension should remain 1 for all remaining
    // levels.
    if (mipWidth > 1)
      mipWidth /= 2;
    if (mipHeight > 1)
      mipHeight /= 2;
  }

  barrier.subresourceRange.baseMipLevel = mipLevels - 1;
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  // Before ending the command buffer, we insert one pipeline barrier. This
  // barrier transitions the last mip level from TRANSFER_DST_OPTIMAL to
  // SHADER_READ_ONLY_OPTIMAL. This wasn't handled by the loop since the last
  // mip level is never blitted from (loop was offset by 1).
  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier);

  endSingleTimeCommands(commandBuffer);
}

// We'll load an image and upload it into a Vulkan image object, using command
// buffers.
inline void create_texture_image(std::string TEXTURE_PATH) {
  int texWidth, texHeight, texChannels;
  // path, out variables for width, height, actual channels channels and then
  // the number of cahnnels to load.
  // STBI_rgb_alpha forces loading with alpha channel even if it is not
  // present. Pointer returned is the first element in the array of pixel
  // values. The pixels are laid out row by row with 4 bytes per pixel. In the
  // case of STBI_rgb_alpha for a total of texWidth * texHeight * 4 values
  stbi_uc *pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight,
                              &texChannels, STBI_rgb_alpha);
  VkDeviceSize imageSize = texWidth * texHeight * 4;
  // Calculate number of levels in the mip chain
  // Select largest dimension, get number of times it can be divided by 2
  // Get the largest whole number of the result.
  mipLevels = static_cast<std::uint32_t>(
      std::floor(std::log2(std::max(texWidth, texHeight))));

  if (!pixels) {
    throw std::runtime_error("failed to load texture image!");
  }

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  // HOST Visible memory so we can map it and use as a transfer
  // source so it can be later copied to an image
  createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               stagingBuffer, stagingBufferMemory);

  // copy the pixel values
  void *data;
  vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
  // clang-format off
    // Reason: Show Hierarchy
      memcpy(data, pixels, static_cast<std::size_t>(imageSize));
  // clang-format on
  vkUnmapMemory(device, stagingBufferMemory);

  stbi_image_free(pixels); // frees image array

  // Format - use same format as the pixels in the buffer, else copy operation
  // will fail. Tiling - We choose for texels are laid out in an implentation
  // defined order
  // (TILING_OPTIMAL) for optimal access and not TILING_LINEAR( row-major in
  // our pixel array), since we are using a staging buffer we can use optimal
  // access.
  // Usage - image is going to be used as destination
  // for the buffer copy. We would also access the image from the shader to
  // color our mesh properties -
  createImage(texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT,
              VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                  VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage,
              textureImageMemory);

  // Copy staging buffer to the texture image
  // Transition the texture image
  // Specify the undefined since it was created with that and we don't care
  // about its contents before the copy operation.
  transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
  // Execute the buffer to image copy
  copyBufferToImage(stagingBuffer, textureImage,
                    static_cast<std::uint32_t>(texWidth),
                    static_cast<std::uint32_t>(texHeight));

  // Transitioned  from VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL to
  // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps (after
  // the blit command reading from it is finished)

  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);

  generateMipmaps(textureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight,
                  mipLevels);
}

} // namespace ice

#endif */