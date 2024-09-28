#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include "config.hpp"
#include "game_objects.hpp"
#include "loaders.hpp"
#include "mesh.hpp"

namespace ice {

enum ConfigFlags : uint32_t {
  VERTEX_SHADER = 1u << 0,
  FRAGMENT_SHADER = 1u << 1,
  VERTEX_INPUT = 1u << 2,
  INPUT_ASSEMBLY = 1u << 3,
  VIEWPORT = 1u << 4,
  SCISSOR = 1u << 5,
  DYNAMIC_STATE = 1u << 6,
  RASTERIZATION = 1u << 7,
  MULTISAMPLE = 1u << 8,
  DEPTH_STENCIL = 1u << 9,
  COLOR_BLEND = 1u << 10,
  PIPELINE_LAYOUT = 1u << 11,
  RENDER_PASS = 1u << 12
};

class GraphicsPipelineBuilder {
 public:
  explicit GraphicsPipelineBuilder(vk::Device device);
  ~GraphicsPipelineBuilder();

  // Reset the builder to its initial state
  GraphicsPipelineBuilder& reset();

  // Shader stage
  GraphicsPipelineBuilder& set_vertex_shader(const std::string& filepath);
  GraphicsPipelineBuilder& set_fragment_shader(const std::string& filepath);

  // state setters
  GraphicsPipelineBuilder& set_vertex_input_state() {
    // empty vertex input state
    config_flags_ |= ConfigFlags::VERTEX_INPUT;
    return *this;
  }
  GraphicsPipelineBuilder& set_vertex_input_state(
      const vk::VertexInputBindingDescription& binding_description,
      const std::vector<vk::VertexInputAttributeDescription>&
          attribute_descriptions);

  GraphicsPipelineBuilder& set_input_assembly_state(
      vk::PrimitiveTopology topology);

  GraphicsPipelineBuilder& set_viewport(const vk::Viewport& viewport);
  GraphicsPipelineBuilder& set_scissor(const vk::Rect2D& scissor);

  GraphicsPipelineBuilder& set_rasterization_state(vk::PolygonMode polygon_mode,
                                                   vk::CullModeFlags cull_mode,
                                                   vk::FrontFace front_face);

  GraphicsPipelineBuilder& set_multisample_state(
      vk::SampleCountFlagBits samples);

  GraphicsPipelineBuilder& enable_depth_test(bool depth_write_enable,
                                             vk::CompareOp compare_op);
  GraphicsPipelineBuilder& disable_depth_test();

  GraphicsPipelineBuilder& set_color_blend_state(
      const std::vector<vk::PipelineColorBlendAttachmentState>&
          color_blend_attachments);
  GraphicsPipelineBuilder& enable_alpha_blending();
  GraphicsPipelineBuilder& disable_blending();

  GraphicsPipelineBuilder& set_dynamic_state(
      const std::vector<vk::DynamicState>& dynamic_states);

  GraphicsPipelineBuilder& set_pipeline_layout(
      vk::PipelineLayout pipeline_layout);

  GraphicsPipelineBuilder& set_render_pass(vk::RenderPass render_pass,
                                           uint32_t subpass = 0);

  // Build the pipeline
  vk::Pipeline build();

 private:
  std::uint32_t config_flags_ = 0;
  static constexpr std::uint32_t REQUIRED_FLAGS =
      (1u << 13) - 1;  // All flags set

  vk::Device device_;

  // Pipeline state
  std::vector<vk::PipelineShaderStageCreateInfo> shader_stages_;
  vk::PipelineVertexInputStateCreateInfo vertex_input_state_;
  vk::PipelineInputAssemblyStateCreateInfo input_assembly_state_;
  vk::PipelineViewportStateCreateInfo viewport_state_;
  vk::PipelineRasterizationStateCreateInfo rasterization_state_;
  vk::PipelineMultisampleStateCreateInfo multisample_state_;
  vk::PipelineDepthStencilStateCreateInfo depth_stencil_state_;
  vk::PipelineColorBlendStateCreateInfo color_blend_state_;

  std::vector<vk::ShaderModule> shader_modules_;
  std::vector<vk::PipelineColorBlendAttachmentState> color_blend_attachments_;

  vk::PipelineDynamicStateCreateInfo dynamic_state_;
  vk::PipelineLayout pipeline_layout_;
  vk::RenderPass render_pass_;
  uint32_t subpass_{};

// Debug Util
#ifndef NDEBUG
  void print_missing_states() const;
#endif
};

inline vk::PipelineLayout make_pipeline_layout(
    const vk::Device& device,
    const std::vector<vk::DescriptorSetLayout>& descriptor_set_layouts) {
#ifndef NDEBUG
  std::cout << "Making pipeline layout" << std::endl;
#endif
  const vk::PipelineLayoutCreateInfo layout_info{
      .setLayoutCount =
          static_cast<std::uint32_t>(descriptor_set_layouts.size()),
      .pSetLayouts = descriptor_set_layouts.data(),
      .pushConstantRangeCount = 0,
  };

  try {
    return device.createPipelineLayout(layout_info);
  } catch (const vk::SystemError& err) {
    throw std::runtime_error("Failed to create pipeline layout!");
  }
}

inline vk::RenderPass make_imgui_renderpass(
    const vk::Device& device, const vk::Format& swapchain_image_format,
    vk::AttachmentLoadOp load_op = vk::AttachmentLoadOp::eLoad,
    vk::ImageLayout initial_layout = vk::ImageLayout::eColorAttachmentOptimal) {
#ifndef NDEBUG
  std::cout << "\nMaking ImGui Renderpass" << std::endl;
#endif

  const vk::AttachmentDescription color_attachment = {
      .format = swapchain_image_format,
      .samples = vk::SampleCountFlagBits::e1,
      .loadOp = load_op,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
      .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
      .initialLayout = initial_layout,
      .finalLayout = vk::ImageLayout::ePresentSrcKHR};

  const vk::AttachmentReference color_attachment_ref = {
      .attachment = 0, .layout = vk::ImageLayout::eColorAttachmentOptimal};

  const vk::SubpassDescription subpass = {
      .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
      .colorAttachmentCount = 1,
      .pColorAttachments = &color_attachment_ref};

  const vk::SubpassDependency dependency = {
      .srcSubpass = VK_SUBPASS_EXTERNAL,
      .dstSubpass = 0,
      .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
      .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
      .srcAccessMask = vk::AccessFlagBits::eNone,
      .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
  };

  const vk::RenderPassCreateInfo renderpass_info = {
      .attachmentCount = 1,
      .pAttachments = &color_attachment,
      .subpassCount = 1,
      .pSubpasses = &subpass,
      .dependencyCount = 1,
      .pDependencies = &dependency,
  };

  try {
    return device.createRenderPass(renderpass_info);
  } catch (const vk::SystemError& err) {
    throw std::runtime_error("Failed to create ImGui renderpass!");
  }
}

inline vk::RenderPass make_scene_renderpass(
    const vk::Device& device, const vk::Format swapchain_image_format,
    const vk::Format swapchain_depth_format, vk::AttachmentLoadOp load_op,
    vk::ImageLayout initial_layout, vk::SampleCountFlagBits msaa_samples) {
#ifndef NDEBUG
  std::cout << "\nMaking Scene Renderpass" << std::endl;
#endif
  std::vector<vk::AttachmentDescription> attachments;
  std::vector<vk::AttachmentReference> color_attachment_refs;

  const vk::AttachmentDescription color_attachment = {
      .format = swapchain_image_format,
      .samples = msaa_samples,
      .loadOp = load_op,
      .storeOp = vk::AttachmentStoreOp::eDontCare,
      .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
      .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
      .initialLayout = initial_layout,
      .finalLayout = vk::ImageLayout::eColorAttachmentOptimal};
  attachments.push_back(color_attachment);
  color_attachment_refs.push_back(
      {0, vk::ImageLayout::eColorAttachmentOptimal});

  const vk::AttachmentDescription depth_attachment = {
      .format = swapchain_depth_format,
      .samples = msaa_samples,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eDontCare,
      .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
      .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
      .initialLayout = vk::ImageLayout::eUndefined,
      .finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal};
  attachments.push_back(depth_attachment);
  const vk::AttachmentReference depth_attachment_ref = {
      .attachment = 1,
      .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal};

  const vk::AttachmentDescription resolve_attachment = {
      .format = swapchain_image_format,
      .samples = vk::SampleCountFlagBits::e1,
      .loadOp = vk::AttachmentLoadOp::eDontCare,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
      .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
      .initialLayout = vk::ImageLayout::eUndefined,
      .finalLayout = vk::ImageLayout::eColorAttachmentOptimal};
  attachments.push_back(resolve_attachment);
  const vk::AttachmentReference resolve_attachment_ref = {
      2, vk::ImageLayout::eColorAttachmentOptimal};

#ifndef NDEBUG
  std::cout << "Attachments:\n";
  for (int index{0}; const auto& attachment : attachments) {
    std::cout << std::format("Attachment {}  \n", index);

    std::cout << std::format(
        "Image format : {}\n"
        "Initial layout : {}\n"
        "Final   layout : {}\n"
        "loadOp         : {}\n"
        "StoreOp        : {}\n"
        "MSAA samples   : {}\n",
        vk::to_string(attachment.format),
        vk::to_string(attachment.initialLayout),
        vk::to_string(attachment.finalLayout), vk::to_string(attachment.loadOp),
        vk::to_string(attachment.storeOp), vk::to_string(attachment.samples));
    ++index;
  }
#endif

  const vk::SubpassDescription subpass = {
      .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
      .colorAttachmentCount =
          static_cast<std::uint32_t>(color_attachment_refs.size()),
      .pColorAttachments = color_attachment_refs.data(),
      .pResolveAttachments = &resolve_attachment_ref,
      .pDepthStencilAttachment = &depth_attachment_ref};

  const vk::SubpassDependency dependency = {
      .srcSubpass = vk::SubpassExternal,
      .dstSubpass = 0,
      .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
      .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
      .srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
      .dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead};

#ifndef NDEBUG
  std::cout << std::format(
      "Renderpass Details\n"
      "Attachments      : {}\n"
      "Attachments Refs : {}\n\n",
      attachments.size(), color_attachment_refs.size());
  std::cout << std::format(
      "Renderpass Details\n"
      "Attachments      : {}\n"
      "Attachments Refs : {}\n\n",
      attachments.size(), color_attachment_refs.size());

#endif

  const vk::RenderPassCreateInfo renderpass_info{
      .attachmentCount = static_cast<std::uint32_t>(attachments.size()),
      .pAttachments = attachments.data(),
      .subpassCount = 1,
      .pSubpasses = &subpass,
      .dependencyCount = 1,
      .pDependencies = &dependency};

  return device.createRenderPass(renderpass_info);
}

inline vk::RenderPass make_sky_renderpass(
    const vk::Device& device, const vk::Format swapchain_image_format,
    vk::AttachmentLoadOp load_op = vk::AttachmentLoadOp::eDontCare,
    vk::ImageLayout initial_layout = vk::ImageLayout::eUndefined,
    vk::SampleCountFlagBits msaa_samples = vk::SampleCountFlagBits::e1) {
#ifndef NDEBUG
  std::cout << "\nMaking Sky Renderpass" << std::endl;
#endif

  std::vector<vk::AttachmentDescription> attachments;
  std::vector<vk::AttachmentReference> color_attachment_refs;

  const vk::AttachmentDescription color_attachment = {
      .format = swapchain_image_format,
      .samples = msaa_samples,
      .loadOp = load_op,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
      .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
      .initialLayout = initial_layout,
      .finalLayout = vk::ImageLayout::eColorAttachmentOptimal};

#ifndef NDEBUG
  std::cout << "Attachment 0  : Color Attachment:\n";

  std::cout << std::format(
      "Initial layout : {}\n"
      "Final   layout : {}\n"
      "loadOp         : {}\n"
      "StoreOp        : {}\n"
      "MSAA samples   : {}\n",
      vk::to_string(color_attachment.initialLayout),
      vk::to_string(color_attachment.finalLayout),
      vk::to_string(color_attachment.loadOp),
      vk::to_string(color_attachment.storeOp),
      vk::to_string(color_attachment.samples));
  std::cout << std::format(
      "Initial layout : {}\n"
      "Final   layout : {}\n"
      "loadOp         : {}\n"
      "StoreOp        : {}\n"
      "MSAA samples   : {}\n",
      vk::to_string(color_attachment.initialLayout),
      vk::to_string(color_attachment.finalLayout),
      vk::to_string(color_attachment.loadOp),
      vk::to_string(color_attachment.storeOp),
      vk::to_string(color_attachment.samples));
#endif

  attachments.push_back(color_attachment);
  const vk::AttachmentReference color_attachment_ref = {
      .attachment = 0, .layout = vk::ImageLayout::eColorAttachmentOptimal};

  color_attachment_refs.push_back(color_attachment_ref);

  // pass attachment refs
  const vk::SubpassDescription subpass = {
      .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
      .colorAttachmentCount = static_cast<std::uint32_t>(attachments.size()),
      .pColorAttachments = &color_attachment_ref,
  };

#ifndef NDEBUG
  std::cout << std::format(
      "Renderpass Details\n"
      "Attachments      : {}\n"
      "Attachments Refs : {}\n\n",
      attachments.size(), color_attachment_refs.size());
  std::cout << std::format(
      "Renderpass Details\n"
      "Attachments      : {}\n"
      "Attachments Refs : {}\n\n",
      attachments.size(), color_attachment_refs.size());
#endif

  const vk::RenderPassCreateInfo renderpass_info{
      .attachmentCount = static_cast<std::uint32_t>(attachments.size()),
      .pAttachments = attachments.data(),
      .subpassCount = 1,
      .pSubpasses = &subpass};
  try {
    return device.createRenderPass(renderpass_info);
  } catch (const vk::SystemError& err) {
    throw std::runtime_error("Failed to create renderpass!");
  }
}
}  // namespace ice
#endif  // PIPELINE_HPP
