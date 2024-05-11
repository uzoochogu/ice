#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include "config.hpp"
#include "game_objects.hpp"
#include "loaders.hpp"
#include "mesh.hpp"

namespace ice {

struct GraphicsPipelineInBundle {
  vk::Device device;
  std::string vertex_filepath;
  std::string fragment_filepath;
  vk::Extent2D swapchain_extent;
  vk::Format swapchain_image_format, swapchain_depth_format;
  std::vector<vk::DescriptorSetLayout> descriptor_set_layouts;
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
                const vk::Format &swapchain_image_format,
                const vk::Format &swapchain_depth_format) {
#ifndef NDEBUG
  std::cout << "Making Renderpass" << std::endl;
#endif

  // Color attachments
  vk::AttachmentDescription color_attachment{
      .format = swapchain_image_format,
      .samples = vk::SampleCountFlagBits::e1,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
      .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
      .initialLayout = vk::ImageLayout::eUndefined,
      .finalLayout = vk::ImageLayout::ePresentSrcKHR};

  vk::AttachmentReference color_attachment_ref{
      .attachment = 0, .layout = vk::ImageLayout::eColorAttachmentOptimal};

  // Depth Attachment
  vk::AttachmentDescription depth_attachment{
      .format = swapchain_depth_format,
      .samples = vk::SampleCountFlagBits::e1,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eDontCare,
      .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
      .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
      .initialLayout = vk::ImageLayout::eUndefined,
      .finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal};

  vk::AttachmentReference depth_attachment_ref{
      .attachment = 1,
      .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal};

  // collate attachments
  std::vector<vk::AttachmentDescription> attachments = {color_attachment,
                                                        depth_attachment};

  // pass attachment refs
  vk::SubpassDescription subpass = {
      .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
      .colorAttachmentCount = 1,
      .pColorAttachments = &color_attachment_ref,
      .pDepthStencilAttachment = &depth_attachment_ref};

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
  vk::VertexInputBindingDescription binding_description =
      Vertex::get_binding_description();
  std::vector<vk::VertexInputAttributeDescription> attribute_descriptions =
      Vertex::get_attribute_descriptions();

  vk::PipelineVertexInputStateCreateInfo vertex_input_info = {
      .flags = vk::PipelineVertexInputStateCreateFlags(),
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = &binding_description,
      .vertexAttributeDescriptionCount =
          static_cast<std::uint32_t>(attribute_descriptions.size()),
      .pVertexAttributeDescriptions = attribute_descriptions.data()};
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
  // Dynamic state in command buffer
  /*   std::vector<vk::DynamicState> dynamic_states =
    {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamic_state_info{
        .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
        .pDynamicStates = dynamic_states.data()};

    // Specify their count at pipeline  creation time
    // The actual viewport and scissor rectangle will be later set up at drawing
    // time
    vk::PipelineViewportStateCreateInfo viewport_state{.viewportCount = 1,
                                                       .scissorCount = 1};
   */
  // Viewport and Scissor
  vk::Viewport viewport = {
      .x = 0.0f,
      .y = 0.0f,
      .width = (float)specification.swapchain_extent.width,
      .height = (float)specification.swapchain_extent.height,
      .minDepth = 0.0f,
      .maxDepth = 1.0f,
  };

  vk::Rect2D scissor = {
      .offset{
          .x = 0,
          .y = 0,
      },
      .extent = specification.swapchain_extent,
  };
  vk::PipelineViewportStateCreateInfo viewport_state = {
      .viewportCount = 1,
      .pViewports = &viewport,
      .scissorCount = 1,
      .pScissors = &scissor,
  };
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
      specification.swapchain_depth_format);
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