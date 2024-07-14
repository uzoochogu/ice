#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include "config.hpp"
#include "game_objects.hpp"
#include "loaders.hpp"
#include "mesh.hpp"

namespace ice {

struct GraphicsPipelineInBundle {
  vk::Device device;
  std::optional<std::vector<vk::VertexInputAttributeDescription>>
      attribute_descriptions;
  std::optional<vk::VertexInputBindingDescription> binding_description;
  std::string vertex_filepath;
  std::string fragment_filepath;
  vk::Extent2D swapchain_extent;
  std::vector<vk::Format> swapchain_image_format;
  std::optional<vk::Format> swapchain_depth_format;
  std::vector<vk::DescriptorSetLayout> descriptor_set_layouts;
  /*  bool should_overwrite; */
  vk::AttachmentLoadOp load_op{vk::AttachmentLoadOp::eClear};
  /* vk::AttachmentStoreOp store_op {vk::AttachmentStoreOp::eStore} */;
  vk::ImageLayout initial_layout{vk::ImageLayout::eUndefined};
  /* vk::ImageLayout final_layout{vk::ImageLayout::ePresentSrcKHR} */;
};

struct GraphicsPipelineOutBundle {
  vk::PipelineLayout layout;
  vk::RenderPass renderpass;
  vk::Pipeline pipeline;
};

inline vk::PipelineLayout make_pipeline_layout(
    const vk::Device &device,
    const std::vector<vk::DescriptorSetLayout> &descriptor_set_layouts) {
#ifndef NDEBUG
  std::cout << "Making pipeline layout" << std::endl;
#endif
  vk::PipelineLayoutCreateInfo layout_info{
      .setLayoutCount =
          static_cast<std::uint32_t>(descriptor_set_layouts.size()),
      .pSetLayouts = descriptor_set_layouts.data(),
      .pushConstantRangeCount = 0,
  };

  try {
    return device.createPipelineLayout(layout_info);
  } catch (vk::SystemError err) {
    throw std::runtime_error("Failed to create pipeline layout!");
  }
}

inline vk::RenderPass
make_renderpass(const vk::Device &device,
                const std::vector<vk::Format> &swapchain_image_format,
                const std::optional<vk::Format> &swapchain_depth_format,   vk::AttachmentLoadOp load_op = vk::AttachmentLoadOp::eDontCare,
  /* vk::AttachmentStoreOp store_op = vk::AttachmentStoreOp::eDontCare, */
  vk::ImageLayout initial_layout = vk::ImageLayout::eUndefined/* ,
  vk::ImageLayout final_layout = vk::ImageLayout::ePresentSrcKHR */) {
#ifndef NDEBUG
  std::cout << "Making Renderpass" << std::endl;
#endif

  // To collate attachments
  std::vector<vk::AttachmentDescription> attachments;

  // Color attachments
  attachments.reserve(swapchain_image_format.size());
  std::vector<vk::AttachmentReference> color_attachment_refs;
  color_attachment_refs.reserve(swapchain_image_format.size());

  std::uint32_t attachment_index{0};
  for (vk::Format color_image_format : swapchain_image_format) {
    vk::AttachmentDescription color_attachment = {
        .format = color_image_format,
        .samples = vk::SampleCountFlagBits::e1,
        .loadOp = load_op,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout = initial_layout,
        .finalLayout = vk::ImageLayout::ePresentSrcKHR};

    attachments.emplace_back(color_attachment);

    vk::AttachmentReference color_attachment_ref = {
        .attachment = attachment_index,
        .layout = vk::ImageLayout::eColorAttachmentOptimal};
    color_attachment_refs.emplace_back(color_attachment_ref);

    ++attachment_index;
  }

  // Depth Attachment
  vk::AttachmentReference depth_attachment_ref{};
  bool depth_present = swapchain_depth_format.has_value();
  if (depth_present) {
    vk ::AttachmentDescription depth_attachment{
        .format = swapchain_depth_format.value(),
        .samples = vk::SampleCountFlagBits::e1,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eDontCare,
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout = vk::ImageLayout::eUndefined,
        .finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal};

    attachments.emplace_back(depth_attachment);

    depth_attachment_ref = {
        .attachment = attachment_index,
        .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal};
  }

  // pass attachment refs
  vk::SubpassDescription subpass = {
      .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
      .colorAttachmentCount =
          static_cast<std::uint32_t>(color_attachment_refs.size()),
      .pColorAttachments = color_attachment_refs.data(),
      .pDepthStencilAttachment =
          depth_present ? &depth_attachment_ref : nullptr};

  vk::RenderPassCreateInfo renderpass_info{
      .attachmentCount = static_cast<std::uint32_t>(attachments.size()),
      .pAttachments = attachments.data(),
      .subpassCount = 1,
      .pSubpasses = &subpass};
  try {
    return device.createRenderPass(renderpass_info);
  } catch (vk::SystemError err) {
    throw std::runtime_error("Failed to create renderpass!");
  }
}

inline GraphicsPipelineOutBundle
make_graphics_pipeline(const GraphicsPipelineInBundle &specification) {
#ifndef NDEBUG
  std::cout << "Making Graphics pipeline" << std::endl;
#endif
  // Will be filled after creation of each stage
  vk::GraphicsPipelineCreateInfo pipeline_info{.flags =
                                                   vk::PipelineCreateFlags()};

  // Vertex input
  // Get vertex data binding descriptions
  bool vertex_attributes_present =
      specification.attribute_descriptions.has_value();
  bool vertex_bindings_present = specification.binding_description.has_value();

  vk::PipelineVertexInputStateCreateInfo vertex_input_info{};
  if (vertex_attributes_present && vertex_bindings_present) {
    /*   vk::VertexInputBindingDescription binding_description =
          Vertex::get_binding_description();
      std::vector<vk::VertexInputAttributeDescription> attribute_descriptions =
          Vertex::get_attribute_descriptions(); */

    vertex_input_info = {
        .flags = vk::PipelineVertexInputStateCreateFlags(),
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions =
            &specification.binding_description.value(),
        .vertexAttributeDescriptionCount = static_cast<std::uint32_t>(
            specification.attribute_descriptions.value().size()),
        .pVertexAttributeDescriptions =
            specification.attribute_descriptions.value().data()};
  }
  pipeline_info.pVertexInputState = &vertex_input_info;

  // Input Assembly
  vk::PipelineInputAssemblyStateCreateInfo input_assembly_info = {
      .topology = vk::PrimitiveTopology::eTriangleList,
      .primitiveRestartEnable = vk::False,
  };
  pipeline_info.pInputAssemblyState = &input_assembly_info;

// Vertex Shader
// will be appended to pipeline later
#ifndef NDEBUG
  std::cout << "Vertex Shader creation" << std::endl;
#endif
  vk::ShaderModule vertex_shader =
      create_shader_module(specification.vertex_filepath, specification.device);
  vk::PipelineShaderStageCreateInfo vertex_shader_info = {
      .stage = vk::ShaderStageFlagBits::eVertex,
      .module = vertex_shader,
      .pName = "main"};

  // Viewport and Scissor
  // Dynamic states to be modified in command buffer - at drawing time
  std::vector<vk::DynamicState> dynamic_states = {vk::DynamicState::eViewport,
                                                  vk::DynamicState::eScissor};
  vk::PipelineDynamicStateCreateInfo dynamic_state_info{
      .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
      .pDynamicStates = dynamic_states.data()};

  vk::PipelineViewportStateCreateInfo viewport_state{.viewportCount = 1,
                                                     .scissorCount = 1};
  pipeline_info.pDynamicState = &dynamic_state_info;
  pipeline_info.pViewportState = &viewport_state;

  // Rasterizer
  vk::PipelineRasterizationStateCreateInfo rasterizer_info{
      .depthClampEnable = vk::False,
      .rasterizerDiscardEnable = vk::False,
      .polygonMode = vk::PolygonMode::eFill,
      .cullMode = vk::CullModeFlagBits::eBack,
      .frontFace = vk::FrontFace::eCounterClockwise, /* eClockwise*/
      .depthBiasEnable = vk::False,
      .depthBiasConstantFactor = 0.0f,
      .depthBiasClamp = 0.0f,
      .depthBiasSlopeFactor = 0.0f,
      .lineWidth = 1.0f,
  };
  pipeline_info.pRasterizationState = &rasterizer_info;

// fragment shader
// append all shader stages
#ifndef NDEBUG
  std::cout << "Fragment Shader creation" << std::endl;
#endif
  vk::ShaderModule fragment_shader = create_shader_module(
      specification.fragment_filepath, specification.device);
  vk::PipelineShaderStageCreateInfo fragment_shader_info = {
      .stage = vk::ShaderStageFlagBits::eFragment,
      .module = fragment_shader,
      .pName = "main"};
  // submit shaders
  std::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages = {
      vertex_shader_info, fragment_shader_info};
  pipeline_info.stageCount = static_cast<uint32_t>(shader_stages.size());
  pipeline_info.pStages = shader_stages.data();

  // multisampling
  vk::PipelineMultisampleStateCreateInfo multisamping_info{
      .rasterizationSamples = vk::SampleCountFlagBits::e1,
      .sampleShadingEnable = vk::False,
  };
  pipeline_info.pMultisampleState = &multisamping_info;

  // Color blend
  vk::PipelineColorBlendAttachmentState color_blend_attachment{
      .blendEnable = vk::False,
      .colorWriteMask =
          vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
  };
  vk::PipelineColorBlendStateCreateInfo color_blending_info = {
      .flags = vk::PipelineColorBlendStateCreateFlags(),
      .logicOpEnable = VK_FALSE,
      .logicOp = vk::LogicOp::eCopy,
      .attachmentCount = 1,
      .pAttachments = &color_blend_attachment,
      .blendConstants = std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f}};

  pipeline_info.pColorBlendState = &color_blending_info;

  // depth stencil
  vk::PipelineDepthStencilStateCreateInfo depth_stencil_info{
      .depthTestEnable = vk::True,
      .depthWriteEnable = vk::True,
      .depthCompareOp = vk::CompareOp::eLess,
      .depthBoundsTestEnable = vk::False,
      .stencilTestEnable = vk::False,
  };
  pipeline_info.pDepthStencilState = &depth_stencil_info;

  // Pipeline layout
  vk::PipelineLayout pipeline_layout = make_pipeline_layout(
      specification.device, specification.descriptor_set_layouts);
  pipeline_info.layout = pipeline_layout;

  //  Renderpass
  vk::RenderPass renderpass = make_renderpass(
      specification.device, specification.swapchain_image_format,
      specification.swapchain_depth_format, specification.load_op,
      /* specification.store_op, */ specification.initial_layout/* ,
      specification.final_layout */);
  pipeline_info.renderPass = renderpass;
  pipeline_info.subpass = 0;

  // Good for derivative pipelines
  pipeline_info.basePipelineHandle = nullptr;

  // Create pipeline
  // Graphics pipeline
  vk::Pipeline graphics_pipeline =
      specification.device.createGraphicsPipeline(nullptr, pipeline_info).value;
  if (graphics_pipeline == nullptr) {
    std::cerr << "Failed to create Pipeline" << std::endl;
  }

  GraphicsPipelineOutBundle output{.layout = pipeline_layout,
                                   .renderpass = renderpass,
                                   .pipeline = graphics_pipeline};

  // clean shader modules
  specification.device.destroyShaderModule(vertex_shader);
  specification.device.destroyShaderModule(fragment_shader);

  return output;
}

} // namespace ice
#endif