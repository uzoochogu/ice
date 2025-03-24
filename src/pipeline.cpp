#include "pipeline.hpp"

#include "loaders.hpp"

namespace ice {

GraphicsPipelineBuilder::GraphicsPipelineBuilder(vk::Device device)
    : device_(device) {
  reset();
}

GraphicsPipelineBuilder::~GraphicsPipelineBuilder() = default;

GraphicsPipelineBuilder& GraphicsPipelineBuilder::reset() {
  config_flags_ = 0;

  shader_stages_.clear();
  vertex_input_state_ = vk::PipelineVertexInputStateCreateInfo();
  input_assembly_state_ = vk::PipelineInputAssemblyStateCreateInfo();
  viewport_state_ = vk::PipelineViewportStateCreateInfo();
  rasterization_state_ = vk::PipelineRasterizationStateCreateInfo();
  multisample_state_ = vk::PipelineMultisampleStateCreateInfo();
  depth_stencil_state_ = vk::PipelineDepthStencilStateCreateInfo();
  color_blend_state_ = vk::PipelineColorBlendStateCreateInfo();
  dynamic_state_ = vk::PipelineDynamicStateCreateInfo();
  pipeline_layout_ = vk::PipelineLayout();
  render_pass_ = vk::RenderPass();
  subpass_ = 0;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_vertex_shader(
    const std::string& filepath) {
  auto shader_module = create_shader_module(filepath, device_);
  shader_modules_.push_back(shader_module);
  shader_stages_.push_back(vk::PipelineShaderStageCreateInfo{
      .stage = vk::ShaderStageFlagBits::eVertex,
      .module = shader_module,
      .pName = "main"});

  config_flags_ |= ConfigFlags::VERTEX_SHADER;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_fragment_shader(
    const std::string& filepath) {
  auto shader_module = create_shader_module(filepath, device_);
  shader_modules_.push_back(shader_module);
  shader_stages_.push_back(vk::PipelineShaderStageCreateInfo{
      .stage = vk::ShaderStageFlagBits::eFragment,
      .module = shader_module,
      .pName = "main"});

  config_flags_ |= ConfigFlags::FRAGMENT_SHADER;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_vertex_input_state(
    const vk::VertexInputBindingDescription& binding_description,
    const std::vector<vk::VertexInputAttributeDescription>&
        attribute_descriptions) {
  vertex_input_state_.setVertexBindingDescriptions(binding_description);
  vertex_input_state_.setVertexAttributeDescriptions(attribute_descriptions);

  config_flags_ |= ConfigFlags::VERTEX_INPUT;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_input_assembly_state(
    vk::PrimitiveTopology topology) {
  input_assembly_state_.topology = topology;
  input_assembly_state_.primitiveRestartEnable = vk::False;

  config_flags_ |= ConfigFlags::INPUT_ASSEMBLY;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_viewport(
    const vk::Viewport& viewport) {
  viewport_state_.setViewports(viewport);

  config_flags_ |= ConfigFlags::VIEWPORT;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_scissor(
    const vk::Rect2D& scissor) {
  viewport_state_.setScissors(scissor);

  config_flags_ |= ConfigFlags::SCISSOR;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_rasterization_state(
    vk::PolygonMode polygon_mode, vk::CullModeFlags cull_mode,
    vk::FrontFace front_face, float line_width) {
  rasterization_state_.polygonMode = polygon_mode;
  rasterization_state_.cullMode = cull_mode;
  rasterization_state_.frontFace = front_face;
  rasterization_state_.depthClampEnable = vk::False;
  rasterization_state_.rasterizerDiscardEnable = vk::False;
  rasterization_state_.depthBiasEnable = vk::False;
  rasterization_state_.lineWidth = line_width;

  config_flags_ |= ConfigFlags::RASTERIZATION;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_multisample_state(
    vk::SampleCountFlagBits samples) {
  multisample_state_.rasterizationSamples = samples;
  multisample_state_.sampleShadingEnable =
      samples == vk::SampleCountFlagBits::e1 ? vk::False : vk::True;
  multisample_state_.minSampleShading = 0.2f;

  config_flags_ |= ConfigFlags::MULTISAMPLE;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::enable_depth_test(
    bool depth_write_enable, vk::CompareOp compare_op) {
  depth_stencil_state_.depthTestEnable = vk::True;
  depth_stencil_state_.depthWriteEnable = depth_write_enable;
  depth_stencil_state_.depthCompareOp = compare_op;
  depth_stencil_state_.depthBoundsTestEnable = vk::False;
  depth_stencil_state_.stencilTestEnable = vk::False;

  config_flags_ |= ConfigFlags::DEPTH_STENCIL;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::disable_depth_test() {
  depth_stencil_state_.depthTestEnable = vk::False;
  depth_stencil_state_.depthWriteEnable = vk::False;

  config_flags_ |= ConfigFlags::DEPTH_STENCIL;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_color_blend_state(
    const std::vector<vk::PipelineColorBlendAttachmentState>&
        color_blend_attachments) {
  color_blend_attachments_ = color_blend_attachments;
  color_blend_state_.setAttachments(color_blend_attachments_);
  color_blend_state_.logicOpEnable = vk::False;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::enable_alpha_blending() {
  vk::PipelineColorBlendAttachmentState color_blend_attachment;
  color_blend_attachment.blendEnable = vk::True;
  color_blend_attachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
  color_blend_attachment.dstColorBlendFactor =
      vk::BlendFactor::eOneMinusSrcAlpha;
  color_blend_attachment.colorBlendOp = vk::BlendOp::eAdd;
  color_blend_attachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
  color_blend_attachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
  color_blend_attachment.alphaBlendOp = vk::BlendOp::eAdd;
  color_blend_attachment.colorWriteMask =
      vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

  color_blend_attachments_ = {
      color_blend_attachment};  // push_back(color_blend_attachment);

  config_flags_ |= ConfigFlags::COLOR_BLEND;
  return set_color_blend_state(color_blend_attachments_);
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::disable_blending() {
  vk::PipelineColorBlendAttachmentState color_blend_attachment;
  color_blend_attachment.blendEnable = vk::False;
  color_blend_attachment.colorWriteMask =
      vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

  color_blend_attachments_ = {
      color_blend_attachment};  // .push_back(color_blend_attachment);

  config_flags_ |= ConfigFlags::COLOR_BLEND;
  return set_color_blend_state(color_blend_attachments_);
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_dynamic_state(
    const std::vector<vk::DynamicState>& dynamic_states) {
  dynamic_state_.setDynamicStates(dynamic_states);

  config_flags_ |= ConfigFlags::DYNAMIC_STATE;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_pipeline_layout(
    vk::PipelineLayout pipeline_layout) {
  pipeline_layout_ = pipeline_layout;

  config_flags_ |= ConfigFlags::PIPELINE_LAYOUT;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_render_pass(
    vk::RenderPass render_pass, uint32_t subpass) {
  render_pass_ = render_pass;
  subpass_ = subpass;

  config_flags_ |= ConfigFlags::RENDER_PASS;
  return *this;
}

#ifndef NDEBUG
void GraphicsPipelineBuilder::print_missing_states() const {
  if (~config_flags_ & ConfigFlags::VERTEX_INPUT) {
    std::cout << "Warning: Vertex Input State Not Defined\n"
                 "If this is intentional, explicitly disable it by calling\n"
                 "set_vertex_input_state() with no arguments to suppress this "
                 "warning\n";
  }
  if (~config_flags_ & ConfigFlags::VERTEX_SHADER) {
    std::cerr << "Error: Vertex shader state missing\n";
  }
  if (~config_flags_ & ConfigFlags::FRAGMENT_SHADER) {
    std::cerr << "Error: Fragment shader  missing\n";
  }
  if (~config_flags_ & ConfigFlags::INPUT_ASSEMBLY) {
    std::cerr << "Error: Input assembly state missing\n";
  }
  if (~config_flags_ & ConfigFlags::VIEWPORT) {
    std::cerr << "Error: Viewport state missing\n";
  }
  if (~config_flags_ & ConfigFlags::RASTERIZATION) {
    std::cerr << "Error: Rasterization state missing\n";
  }
  if (~config_flags_ & ConfigFlags::MULTISAMPLE) {
    std::cerr << "Error: Multisample state missing\n";
  }
  if (~config_flags_ & ConfigFlags::DEPTH_STENCIL) {
    std::cerr << "Error: Depth stencil state missing\n";
  }
  if (~config_flags_ & ConfigFlags::COLOR_BLEND) {
    std::cerr << "Error: Color blend state missing\n";
  }
  if (~config_flags_ & ConfigFlags::DYNAMIC_STATE) {
    std::cerr << "Error: Dynamic state missing\n";
  }
  if (~config_flags_ & ConfigFlags::PIPELINE_LAYOUT) {
    std::cerr << "Error: Pipeline layout missing\n";
  }
  if (~config_flags_ & ConfigFlags::RENDER_PASS) {
    std::cerr << "Error: Render pass missing\n";
  }
}
#endif

vk::Pipeline GraphicsPipelineBuilder::build() {
  if (config_flags_ != REQUIRED_FLAGS) {
#ifndef NDEBUG
    std::cerr << "Warning: Graphics pipeline configuration incomplete\n";
    print_missing_states();
    std::cerr << "Proceeding...\n";
#endif
    // don't exit early.
  }
  vk::GraphicsPipelineCreateInfo pipeline_info;
  pipeline_info.setStages(shader_stages_)
      .setPVertexInputState(&vertex_input_state_)
      .setPInputAssemblyState(&input_assembly_state_)
      .setPViewportState(&viewport_state_)
      .setPRasterizationState(&rasterization_state_)
      .setPMultisampleState(&multisample_state_)
      .setPDepthStencilState(&depth_stencil_state_)
      .setPColorBlendState(&color_blend_state_)
      .setPDynamicState(&dynamic_state_)
      .setLayout(pipeline_layout_)
      .setRenderPass(render_pass_)
      .setSubpass(subpass_);

  const vk::ResultValue<vk::Pipeline> result =
      device_.createGraphicsPipeline(nullptr, pipeline_info);
  if (result.result != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to create graphics pipeline!");
  }

  for (const auto& shader_module : shader_modules_) {
    device_.destroyShaderModule(shader_module);
  }
  shader_modules_.clear();

  return result.value;
}

}  // namespace ice
