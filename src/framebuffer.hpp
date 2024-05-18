#ifndef FRAMEBUFFER_HPP
#define FRAMEBUFFER_HPP

#include "config.hpp"
#include "swapchain.hpp"
namespace ice {

struct FramebufferInput {
  vk::Device device;
  std::unordered_map<PipelineType, vk::RenderPass> renderpass;
  vk::Extent2D swapchain_extent;
};

/**
 * \brief Make framebuffers for the swapchain
 * \param input_bundle  Requirements for creating a framebuffer: device,
 renderpass and swapchain extent
 * \param out_frames out variable `std::vector<SwapChainFrames>` to be populated
 with created framebuffers
*/
inline void make_framebuffers(const FramebufferInput &input_bundle,
                              std::vector<SwapChainFrame> &out_frames) {
  for (int i = 0; i < out_frames.size(); ++i) {
    std::vector<vk::ImageView> attachments = {
        out_frames[i].image_view,
        /* out_frames[i].depth_buffer_view, */
    };

    vk::FramebufferCreateInfo framebuffer_info{
        .renderPass = input_bundle.renderpass.at(PipelineType::SKY),
        .attachmentCount = static_cast<std::uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .width = input_bundle.swapchain_extent.width,
        .height = input_bundle.swapchain_extent.height,
        .layers = 1};

    out_frames[i].framebuffer[PipelineType::SKY] =
        input_bundle.device.createFramebuffer(framebuffer_info);

    if (out_frames[i].framebuffer[PipelineType::SKY] == nullptr) {
      std::cout << std::format("Failed to create Sky framebuffer for frame {}",
                               i)
                << std::endl;
    }

    attachments.push_back(out_frames[i].depth_buffer_view);
    framebuffer_info.renderPass =
        input_bundle.renderpass.at(PipelineType::STANDARD);
    framebuffer_info.attachmentCount =
        static_cast<uint32_t>(attachments.size());
    framebuffer_info.pAttachments = attachments.data();

    out_frames[i].framebuffer[PipelineType::STANDARD] =
        input_bundle.device.createFramebuffer(framebuffer_info);

    if (out_frames[i].framebuffer[PipelineType::STANDARD] == nullptr) {
      std::cout << std::format(
                       "Failed to create standard framebuffer for frame {}", i)
                << std::endl;
    }
  }
}

} // namespace ice

#endif