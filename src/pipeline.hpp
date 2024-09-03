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
  vk::Format swapchain_image_format;
  std::optional<vk::Format> swapchain_depth_format;
  std::vector<vk::DescriptorSetLayout> descriptor_set_layouts;
  vk::AttachmentLoadOp load_op{vk::AttachmentLoadOp::eClear};
  vk::ImageLayout initial_layout{vk::ImageLayout::eUndefined};
  vk::SampleCountFlagBits msaa_samples{vk::SampleCountFlagBits::e1};
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

inline vk::RenderPass make_imgui_renderpass(
    const vk::Device &device, const vk::Format &swapchain_image_format,
    const vk::Format &swapchain_depth_format,
    vk::AttachmentLoadOp load_op = vk::AttachmentLoadOp::eLoad,
    vk::ImageLayout initial_layout = vk::ImageLayout::eColorAttachmentOptimal) {
#ifndef NDEBUG
  std::cout << "Making ImGui Renderpass" << std::endl;
#endif
  vk::AttachmentDescription color_attachment = {
      .format = swapchain_image_format,
      .samples = vk::SampleCountFlagBits::e1,
      .loadOp = load_op,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
      .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
      .initialLayout = initial_layout,
      .finalLayout = vk::ImageLayout::ePresentSrcKHR};

  vk::AttachmentReference colorAttachmentRef = {
      .attachment = 0, .layout = vk::ImageLayout::eColorAttachmentOptimal};

  vk::SubpassDescription subpass = {.pipelineBindPoint =
                                        vk::PipelineBindPoint::eGraphics,
                                    .colorAttachmentCount = 1,
                                    .pColorAttachments = &colorAttachmentRef};

  vk::SubpassDependency dependency = {
      .srcSubpass = VK_SUBPASS_EXTERNAL,
      .dstSubpass = 0,
      .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
      .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
      .srcAccessMask = vk::AccessFlagBits::eNone,
      .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
  };

  vk::RenderPassCreateInfo renderpass_info = {
      .attachmentCount = 1,
      .pAttachments = &color_attachment,
      .subpassCount = 1,
      .pSubpasses = &subpass,
      .dependencyCount = 1,
      .pDependencies = &dependency,
  };

  try {
    return device.createRenderPass(renderpass_info);
  } catch (vk::SystemError err) {
    throw std::runtime_error("Failed to create ImGui renderpass!");
  }
}

inline vk::RenderPass make_scene_renderpass(
    const vk::Device &device, const vk::Format swapchain_image_format,
    const vk::Format swapchain_depth_format, vk::AttachmentLoadOp load_op,
    vk::ImageLayout initial_layout, vk::SampleCountFlagBits msaa_samples) {
#ifndef NDEBUG
  std::cout << "\nMaking Scene Renderpass" << std::endl;
#endif
  std::vector<vk::AttachmentDescription> attachments;
  std::vector<vk::AttachmentReference> color_attachment_refs;

  vk::AttachmentDescription color_attachment = {
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

  vk::AttachmentDescription depth_attachment = {
      .format = swapchain_depth_format,
      .samples = msaa_samples,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eDontCare,
      .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
      .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
      .initialLayout = vk::ImageLayout::eUndefined,
      .finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal};
  attachments.push_back(depth_attachment);
  vk::AttachmentReference depth_attachment_ref = {
      .attachment = 1,
      .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal};

  vk::AttachmentDescription resolve_attachment = {
      .format = swapchain_image_format,
      .samples = vk::SampleCountFlagBits::e1,
      .loadOp = vk::AttachmentLoadOp::eDontCare,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
      .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
      .initialLayout = vk::ImageLayout::eUndefined,
      .finalLayout = vk::ImageLayout::eColorAttachmentOptimal};
  attachments.push_back(resolve_attachment);
  vk::AttachmentReference resolve_attachment_ref = {
      2, vk::ImageLayout::eColorAttachmentOptimal};

#ifndef NDEBUG
  std::cout << "Attachments:\n";
  for (int index{0}; const auto &attachment : attachments) {
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

  vk::SubpassDescription subpass = {
      .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
      .colorAttachmentCount =
          static_cast<std::uint32_t>(color_attachment_refs.size()),
      .pColorAttachments = color_attachment_refs.data(),
      .pResolveAttachments = &resolve_attachment_ref,
      .pDepthStencilAttachment = &depth_attachment_ref};

  vk::SubpassDependency dependency = {
      .srcSubpass = vk::SubpassExternal,
      .dstSubpass = 0,
      .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
      .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
      .srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
      .dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead};

#ifndef NDEBUG
  std::cout << std::format("Renderpass Details\n"
                           "Attachments      : {}\n"
                           "Attachments Refs : {}\n\n",
                           attachments.size(), color_attachment_refs.size());

#endif

  vk::RenderPassCreateInfo renderpass_info{
      .attachmentCount = static_cast<std::uint32_t>(attachments.size()),
      .pAttachments = attachments.data(),
      .subpassCount = 1,
      .pSubpasses = &subpass,
      .dependencyCount = 1,
      .pDependencies = &dependency};

  return device.createRenderPass(renderpass_info);
}

inline vk::RenderPass make_sky_renderpass(
    const vk::Device &device, const vk::Format swapchain_image_format,
    vk::AttachmentLoadOp load_op = vk::AttachmentLoadOp::eDontCare,
    vk::ImageLayout initial_layout = vk::ImageLayout::eUndefined,
    vk::SampleCountFlagBits msaa_samples = vk::SampleCountFlagBits::e1) {
#ifndef NDEBUG
  std::cout << "\nMaking Sky Renderpass" << std::endl;
#endif

  std::vector<vk::AttachmentDescription> attachments;
  std::vector<vk::AttachmentReference> color_attachment_refs;

  vk::AttachmentDescription color_attachment = {
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

  std::cout << std::format("Initial layout : {}\n"
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
  vk::AttachmentReference color_attachment_ref = {
      .attachment = 0, .layout = vk::ImageLayout::eColorAttachmentOptimal};

  color_attachment_refs.push_back(color_attachment_ref);

  // pass attachment refs
  vk::SubpassDescription subpass = {
      .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
      .colorAttachmentCount = static_cast<std::uint32_t>(attachments.size()),
      .pColorAttachments = &color_attachment_ref,
  };

#ifndef NDEBUG
  std::cout << std::format("Renderpass Details\n"
                           "Attachments      : {}\n"
                           "Attachments Refs : {}\n\n",
                           attachments.size(), color_attachment_refs.size());
#endif

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
#ifndef NDEBUG
  std::cout << "Fragment Shader creation" << std::endl;
#endif
  vk::ShaderModule fragment_shader = create_shader_module(
      specification.fragment_filepath, specification.device);
  vk::PipelineShaderStageCreateInfo fragment_shader_info = {
      .stage = vk::ShaderStageFlagBits::eFragment,
      .module = fragment_shader,
      .pName = "main"};
  // submit all shader stages
  std::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages = {
      vertex_shader_info, fragment_shader_info};
  pipeline_info.stageCount = static_cast<uint32_t>(shader_stages.size());
  pipeline_info.pStages = shader_stages.data();

  // multisampling
  vk::PipelineMultisampleStateCreateInfo multisamping_info{
      .rasterizationSamples = specification.msaa_samples,
      .sampleShadingEnable =
          vk::True, // improves image quality, performance cost
      .minSampleShading =
          .2f // Min fraction for sample shading: closer to 1 is smoother
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
      .logicOpEnable = vk::False,
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

  // choose which renderpass to make
  vk::RenderPass renderpass =
      specification.swapchain_depth_format.has_value()
          ? make_scene_renderpass(
                specification.device, specification.swapchain_image_format,
                specification.swapchain_depth_format.value(),
                specification.load_op, specification.initial_layout,
                specification.msaa_samples)
          : make_sky_renderpass(
                specification.device, specification.swapchain_image_format,
                specification.load_op, specification.initial_layout,
                specification.msaa_samples);

  pipeline_info.renderPass = renderpass;
  pipeline_info.subpass = 0;

  pipeline_info.basePipelineHandle = nullptr; // no derivatives

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
