#include "swapchain.hpp"

#include "images/ice_image.hpp"

namespace ice {
void SwapChainFrame::make_descriptor_resources() {
  BufferCreationInput input{
      .size = sizeof(CameraVectors),
      .usage = vk::BufferUsageFlagBits::eUniformBuffer,
      .memory_properties = vk::MemoryPropertyFlagBits::eHostVisible |
                           vk::MemoryPropertyFlagBits::eHostCoherent,
      .logical_device = logical_device,

      .physical_device = physical_device,
  };

  // Camera vector
  camera_vector_buffer = create_buffer(input);
  camera_vector_write_location = logical_device.mapMemory(
      camera_vector_buffer.buffer_memory, 0, sizeof(CameraVectors));

  // Camera Matrix
  input.size = sizeof(CameraMatrices);
  camera_matrix_buffer = create_buffer(input);

  camera_matrix_write_location = logical_device.mapMemory(
      camera_matrix_buffer.buffer_memory, 0, sizeof(CameraMatrices));

  // model data
  constexpr const std::uint32_t INSTANCES = 1024;
  input.size = INSTANCES * sizeof(glm::mat4);
  input.usage = vk::BufferUsageFlagBits::eStorageBuffer;
  model_buffer = create_buffer(input);

  model_buffer_write_location = logical_device.mapMemory(
      model_buffer.buffer_memory, 0, INSTANCES * sizeof(glm::mat4));

  model_transforms.reserve(INSTANCES);
  for (std::uint32_t i = 0; i < INSTANCES; ++i) {
    model_transforms.emplace_back(1.0f);
  }

  camera_vector_descriptor_info = {.buffer = camera_vector_buffer.buffer,
                                   .offset = 0,
                                   .range = sizeof(CameraVectors)};

  camera_matrix_descriptor_info = {.buffer = camera_matrix_buffer.buffer,
                                   .offset = 0,
                                   .range = sizeof(CameraMatrices)};

  ssbo_descriptor_info = {.buffer = model_buffer.buffer,
                          .offset = 0,
                          .range = INSTANCES * sizeof(glm::mat4)};
}

void SwapChainFrame::make_depth_resources() {
  depth_format = ice_image::find_supported_format(
      physical_device, {vk::Format::eD32Sfloat, vk::Format::eD24UnormS8Uint},
      vk::ImageTiling::eOptimal,
      vk::FormatFeatureFlagBits::eDepthStencilAttachment);

  const ice_image::ImageCreationInput image_info{
      .logical_device = logical_device,
      .physical_device = physical_device,
      .width = extent.width,
      .height = extent.height,
      .tiling = vk::ImageTiling::eOptimal,
      .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
      .memory_properties = vk::MemoryPropertyFlagBits::eDeviceLocal,
      .format = depth_format,
      .msaa_samples = msaa_samples};

  depth_buffer = ice_image::make_image(image_info);
  depth_buffer_memory = ice_image::make_image_memory(image_info, depth_buffer);
  depth_buffer_view =
      ice_image::make_image_view(logical_device, depth_buffer, depth_format,
                                 vk::ImageAspectFlagBits::eDepth);
}

void SwapChainFrame::make_color_resources() {
  const ice_image::ImageCreationInput image_info{
      .logical_device = logical_device,
      .physical_device = physical_device,
      .width = extent.width,
      .height = extent.height,
      .tiling = vk::ImageTiling::eOptimal,
      .usage = vk::ImageUsageFlagBits::eColorAttachment |
               vk::ImageUsageFlagBits::eTransientAttachment,
      .memory_properties = vk::MemoryPropertyFlagBits::eDeviceLocal,
      .format = color_format,
      .mip_levels = 1,
      .msaa_samples = msaa_samples};

  color_buffer = ice_image::make_image(image_info);
  color_buffer_memory = ice_image::make_image_memory(image_info, color_buffer);
  color_buffer_view =
      ice_image::make_image_view(logical_device, color_buffer, color_format,
                                 vk::ImageAspectFlagBits::eColor);
}

void SwapChainFrame::record_write_operations() {
  const vk::WriteDescriptorSet camera_vector_write_op = {
      .dstSet = descriptor_sets[PipelineType::SKY],
      .dstBinding = 0,
      .dstArrayElement =
          0,  // byte offset within binding for inline uniform blocks
      .descriptorCount = 1,
      .descriptorType = vk::DescriptorType::eUniformBuffer,
      .pBufferInfo = &camera_vector_descriptor_info};

  const vk::WriteDescriptorSet camera_matrix_write_op = {
      .dstSet = descriptor_sets[PipelineType::STANDARD],
      .dstBinding = 0,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = vk::DescriptorType::eUniformBuffer,
      .pBufferInfo = &camera_matrix_descriptor_info};

  const vk::WriteDescriptorSet ssbo_write_op = {
      .dstSet = descriptor_sets[PipelineType::STANDARD],
      .dstBinding = 1,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = vk::DescriptorType::eStorageBuffer,
      .pBufferInfo = &ssbo_descriptor_info};

  write_ops = {camera_vector_write_op, camera_matrix_write_op, ssbo_write_op};
}

void SwapChainFrame::write_descriptor_set() const {
  logical_device.updateDescriptorSets(write_ops, nullptr);
}

void SwapChainFrame::destroy(vk::CommandPool imgui_command_pool) {
  // image resources
  logical_device.destroyImageView(image_view);
  logical_device.destroyFramebuffer(framebuffer[PipelineType::SKY]);
  logical_device.destroyFramebuffer(framebuffer[PipelineType::STANDARD]);

  // imgui framebuffer
  logical_device.destroyFramebuffer(imgui_framebuffer);
  logical_device.freeCommandBuffers(imgui_command_pool, 1,
                                    &imgui_command_buffer);

  // sync objects
  logical_device.destroyFence(in_flight_fence);
  logical_device.destroySemaphore(image_available);
  logical_device.destroySemaphore(render_finished);

  // camera data
  logical_device.unmapMemory(camera_vector_buffer.buffer_memory);
  logical_device.freeMemory(camera_vector_buffer.buffer_memory);
  logical_device.destroyBuffer(camera_vector_buffer.buffer);
  logical_device.unmapMemory(camera_matrix_buffer.buffer_memory);
  logical_device.freeMemory(camera_matrix_buffer.buffer_memory);
  logical_device.destroyBuffer(camera_matrix_buffer.buffer);

  // obj data
  logical_device.unmapMemory(model_buffer.buffer_memory);
  logical_device.freeMemory(model_buffer.buffer_memory);
  logical_device.destroyBuffer(model_buffer.buffer);

  // depth resources
  logical_device.destroyImage(depth_buffer);
  logical_device.freeMemory(depth_buffer_memory);
  logical_device.destroyImageView(depth_buffer_view);

  // color resources
  logical_device.destroyImage(color_buffer);
  logical_device.freeMemory(color_buffer_memory);
  logical_device.destroyImageView(color_buffer_view);
}

}  // namespace ice
