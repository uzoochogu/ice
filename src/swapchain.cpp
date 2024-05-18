#include "swapchain.hpp"

#include "config.hpp"
#include "images/ice_image.hpp"
#include "ubo.hpp"

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
    model_transforms.push_back(glm::mat4(1.0f));
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

  ice_image::ImageCreationInput image_info{
      .logical_device = logical_device,
      .physical_device = physical_device,
      .width = extent.width,
      .height = extent.height,
      .tiling = vk::ImageTiling::eOptimal,
      .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
      .memory_properties = vk::MemoryPropertyFlagBits::eDeviceLocal,
      .format = depth_format};

  depth_buffer = ice_image::make_image(image_info);
  depth_buffer_memory = ice_image::make_image_memory(image_info, depth_buffer);
  depth_buffer_view =

      ice_image::make_image_view(logical_device, depth_buffer, depth_format,
                                 vk::ImageAspectFlagBits::eDepth);
}

void SwapChainFrame::record_write_operations() {
  vk::WriteDescriptorSet camera_vector_write_op = {
      .dstSet = descriptor_sets[PipelineType::SKY],
      .dstBinding = 0,
      .dstArrayElement =
          0, // byte offset within binding for inline uniform blocks
      .descriptorCount = 1,
      .descriptorType = vk::DescriptorType::eUniformBuffer,
      .pBufferInfo = &camera_vector_descriptor_info};

  vk::WriteDescriptorSet camera_matrix_write_op = {
      .dstSet = descriptor_sets[PipelineType::STANDARD],
      .dstBinding = 0,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = vk::DescriptorType::eUniformBuffer,
      .pBufferInfo = &camera_matrix_descriptor_info};

  vk::WriteDescriptorSet ssbo_write_op = {
      .dstSet = descriptor_sets[PipelineType::STANDARD],
      .dstBinding = 1,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = vk::DescriptorType::eStorageBuffer,
      .pBufferInfo = &ssbo_descriptor_info};

  write_ops = {camera_vector_write_op, camera_matrix_write_op, ssbo_write_op};
}

void SwapChainFrame::write_descriptor_set() {
  logical_device.updateDescriptorSets(write_ops, nullptr);
}

void SwapChainFrame::destroy() {
  // image resources
  logical_device.destroyImageView(image_view);
  logical_device.destroyFramebuffer(framebuffer[PipelineType::SKY]);
  logical_device.destroyFramebuffer(framebuffer[PipelineType::STANDARD]);

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
}

} // namespace ice