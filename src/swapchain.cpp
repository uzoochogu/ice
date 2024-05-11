#include "swapchain.hpp"

#include "images/ice_image.hpp"

namespace ice {
void SwapChainFrame::make_descriptor_resources() {

  BufferCreationInput input{
      .size = sizeof(UBO),
      .usage = vk::BufferUsageFlagBits::eUniformBuffer,
      .memory_properties = vk::MemoryPropertyFlagBits::eHostVisible |
                           vk::MemoryPropertyFlagBits::eHostCoherent,
      .logical_device = logical_device,

      .physical_device = physical_device,
  };

  camera_data_buffer = create_buffer(input);

  camera_data_write_location = logical_device.mapMemory(
      camera_data_buffer.buffer_memory, 0, sizeof(UBO));

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

  uniform_buffer_descriptor_info = {
      .buffer = camera_data_buffer.buffer, .offset = 0, .range = sizeof(UBO)};

  model_buffer_descriptor_info = {.buffer = model_buffer.buffer,
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

void SwapChainFrame::write_descriptor_set() {

  vk::WriteDescriptorSet write_info{
      .dstSet = descriptor_set,
      .dstBinding = 0,
      .dstArrayElement =
          0, // byte offset within binding for inline uniform blocks
      .descriptorCount = 1,
      .descriptorType = vk::DescriptorType::eUniformBuffer,
      .pBufferInfo = &uniform_buffer_descriptor_info};

  logical_device.updateDescriptorSets(write_info, nullptr);

  // transforms write info
  write_info.dstBinding = 1,
  write_info.descriptorType = vk::DescriptorType::eStorageBuffer;
  write_info.pBufferInfo = &model_buffer_descriptor_info;

  logical_device.updateDescriptorSets(write_info, nullptr);
}

void SwapChainFrame::destroy() {
  // image resources
  logical_device.destroyImageView(image_view);
  logical_device.destroyFramebuffer(framebuffer);

  // sync objects
  logical_device.destroyFence(in_flight_fence);
  logical_device.destroySemaphore(image_available);
  logical_device.destroySemaphore(render_finished);

  // camera data
  logical_device.unmapMemory(camera_data_buffer.buffer_memory);
  logical_device.freeMemory(camera_data_buffer.buffer_memory);
  logical_device.destroyBuffer(camera_data_buffer.buffer);

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