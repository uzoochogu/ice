#ifndef FRAMEBUFFER_HPP
#define FRAMEBUFFER_HPP

#include "config.hpp"
#include "swapchain.hpp"
namespace ice {

struct FramebufferInput {
  vk::Device device;
  std::unordered_map<PipelineType, vk::RenderPass> renderpass;
  vk::RenderPass imgui_renderpass;
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
    std::vector<vk::ImageView> attachments = {out_frames[i].color_buffer_view};

    // Sky Pipeline
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
#ifndef NDEBUG
      std::cerr << std::format("Failed to create Sky framebuffer for frame {}",
                               i)
                << std::endl;
#endif
      throw std::runtime_error(
          std::format("Failed to create Sky framebuffer for frame {}", i));
    }

    // Standard Pipeline
    attachments = {out_frames[i].color_buffer_view,
                   out_frames[i].depth_buffer_view, out_frames[i].image_view};
    framebuffer_info.renderPass =
        input_bundle.renderpass.at(PipelineType::STANDARD);
    framebuffer_info.attachmentCount =
        static_cast<uint32_t>(attachments.size());
    framebuffer_info.pAttachments = attachments.data();

    out_frames[i].framebuffer[PipelineType::STANDARD] =
        input_bundle.device.createFramebuffer(framebuffer_info);

    if (out_frames[i].framebuffer[PipelineType::STANDARD] == nullptr) {
#ifndef NDEBUG
      std::cerr << std::format(
                       "Failed to create standard framebuffer for frame {}", i)
                << std::endl;
#endif
      throw std::runtime_error(
          std::format("Failed to create standard framebuffer for frame {}", i));
    }

    // imgui framebuffer
    attachments = {out_frames[i].image_view};
    framebuffer_info.renderPass = input_bundle.imgui_renderpass;
    framebuffer_info.attachmentCount = 1;
    framebuffer_info.pAttachments = attachments.data();

    out_frames[i].imgui_framebuffer =
        input_bundle.device.createFramebuffer(framebuffer_info);

    if (out_frames[i].imgui_framebuffer == nullptr) {
#ifndef NDEBUG
      std::cerr << std::format(
                       "Failed to create ImGui framebuffer for frame {}", i)
                << std::endl;
#endif
      throw std::runtime_error(
          std::format("Failed to create ImGui framebuffer for frame {}", i));
    }
  }
}

}  // namespace ice

#endif  // FRAMEBUFFER_HPP
