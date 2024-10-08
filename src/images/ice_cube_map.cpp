#define STB_IMAGE_IMPLEMENTATION
#include "ice_cube_map.hpp"

#include "../data_buffers.hpp"
#include "../descriptors.hpp"

namespace ice_image {

CubeMap::CubeMap(const TextureCreationInput &input) {
  logical_device = input.logical_device;
  physical_device = input.physical_device;
  filenames = input.filenames;
  command_buffer = input.command_buffer;
  queue = input.queue;
  layout = input.layout;
  descriptor_pool = input.descriptor_pool;

  // Ensure files are FACES_IN_CUBE in number
  const int number_of_files = static_cast<int>(filenames.size());
  if (number_of_files < FACES_IN_CUBE) {
#ifndef NDEBUG
    std::cout << "Inadequate filenames provided, duplicating provided "
                 "filenames.....\n";
#endif
    // duplicate provided files
    for (int i = 0; i < FACES_IN_CUBE - number_of_files; ++i) {
      filenames.push_back(filenames[i % number_of_files]);
    }
#ifndef NDEBUG
    std::cout << "Finished duplicating filenames\n";
#endif
  }

  load();

  const ImageCreationInput image_input{
      .logical_device = logical_device,
      .physical_device = physical_device,
      .width = static_cast<std::uint32_t>(width),
      .height = static_cast<std::uint32_t>(height),
      .tiling = vk::ImageTiling::eOptimal,
      .usage = vk::ImageUsageFlagBits::eTransferDst |
               vk::ImageUsageFlagBits::eSampled,
      .memory_properties = vk::MemoryPropertyFlagBits::eDeviceLocal,
      .format = vk::Format::eR8G8B8A8Srgb,
      .array_count = FACES_IN_CUBE,
      .create_flags = vk::ImageCreateFlagBits::eCubeCompatible};

  image = make_image(image_input);
#ifndef NDEBUG
  std::cout << "Finished Creating the VkImage for Cube Map\n\n" << std::endl;
#endif

  image_memory = make_image_memory(image_input, image);

  populate();

  for (auto &pixel : pixels) {
    stbi_image_free(pixel);
  }

  make_view();

  make_sampler();

  make_descriptor_set();
#ifndef NDEBUG
  std::cout << "Finished Creating the ice_image::CubeMap Object\n\n"
            << std::endl;
#endif
}

CubeMap::~CubeMap() {
  logical_device.freeMemory(image_memory);
  logical_device.destroyImage(image);
  logical_device.destroyImageView(image_view);
  logical_device.destroySampler(sampler);
}

void CubeMap::load() {
#ifndef NDEBUG
  std::cout << "\nLoading CubeMaps.....\n";
#endif
  for (int i = 0; i < FACES_IN_CUBE; i++) {
    pixels[i] = stbi_load(filenames[i].c_str(), &width, &height, &channels,
                          STBI_rgb_alpha);
    if (pixels[i] == nullptr) {
      // Error Recovery
      std::cout << std::format("Unable to load: {}", filenames[i]) << std::endl;
      width = height = 10;
      channels = 4;
#ifndef NDEBUG
      std::cout << std::format(
          "Empty load in image {},  Allocated random image of size {} x {}\n",
          i, width, height);
#endif
      pixels[i] =
          new stbi_uc[static_cast<std::size_t>(width * height * channels)];
      memset(pixels[i], 255,
             static_cast<std::size_t>(width * height) * channels);
    }
  }
}

void CubeMap::populate() {
  const std::uint32_t image_size = width * height * 4;
  // First create a CPU-visible buffer...
  const ice::BufferCreationInput input{
      .size = static_cast<std::size_t>(image_size) * FACES_IN_CUBE,
      .usage = vk::BufferUsageFlagBits::eTransferSrc,

      .memory_properties = vk::MemoryPropertyFlagBits::eHostCoherent |
                           vk::MemoryPropertyFlagBits::eHostVisible,

      .logical_device = logical_device,
      .physical_device = physical_device,
  };

  const ice::BufferBundle staging_buffer = create_buffer(input);

  // fill all of them
  for (int i = 0; i < FACES_IN_CUBE; ++i) {
    void *write_location = logical_device.mapMemory(
        staging_buffer.buffer_memory,
        static_cast<vk::DeviceSize>(image_size) * i, image_size);
    memcpy(write_location, pixels[i], image_size);
    logical_device.unmapMemory(staging_buffer.buffer_memory);
  }

  // then transfer it to image memory
  ImageLayoutTransitionJob transition_job{
      .command_buffer = command_buffer,
      .queue = queue,
      .image = image,
      .old_layout = vk::ImageLayout::eUndefined,
      .new_layout = vk::ImageLayout::eTransferDstOptimal,
      .array_count = FACES_IN_CUBE};

  transition_image_layout(transition_job);

  const BufferImageCopyJob copy_job{.command_buffer = command_buffer,
                                    .queue = queue,
                                    .src_buffer = staging_buffer.buffer,
                                    .dst_image = image,
                                    .width = width,
                                    .height = height,
                                    .array_count = FACES_IN_CUBE};

  copy_buffer_to_image(copy_job);

  transition_job.old_layout = vk::ImageLayout::eTransferDstOptimal;
  transition_job.new_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
  transition_image_layout(transition_job);

  // Now the staging buffer can be destroyed
  logical_device.freeMemory(staging_buffer.buffer_memory);
  logical_device.destroyBuffer(staging_buffer.buffer);
}

void CubeMap::make_view() {
  image_view = make_image_view(logical_device, image, vk::Format::eR8G8B8A8Srgb,
                               vk::ImageAspectFlagBits::eColor,
                               vk::ImageViewType::eCube, FACES_IN_CUBE);
#ifndef NDEBUG
  std::cout << "Finished Creating the image view for Cube Map\n\n" << std::endl;
#endif
}

void CubeMap::make_sampler() {
  const vk::SamplerCreateInfo sampler_info{
      .flags = vk::SamplerCreateFlags(),
      .magFilter = vk::Filter::eLinear,
      .minFilter = vk::Filter::eNearest,
      .mipmapMode = vk::SamplerMipmapMode::eLinear,
      .addressModeU = vk::SamplerAddressMode::eRepeat,
      .addressModeV = vk::SamplerAddressMode::eRepeat,
      .addressModeW = vk::SamplerAddressMode::eRepeat,
      .anisotropyEnable = false,
      .maxAnisotropy = 1.0f,

      .compareEnable = false,
      .compareOp = vk::CompareOp::eAlways,
      .borderColor = vk::BorderColor::eIntOpaqueBlack,
      .unnormalizedCoordinates = false};

  try {
    sampler = logical_device.createSampler(sampler_info);
  } catch (const vk::SystemError &err) {
#ifndef NDEBUG
    std::cout << "Failed to make sampler for Cube Map." << std::endl;
#endif
  }
#ifndef NDEBUG
  std::cout << "Finished Creating the sampler for Cube Map\n\n" << std::endl;
#endif
}

void CubeMap::make_descriptor_set() {
  descriptor_set =
      ice::allocate_descriptor_sets(logical_device, descriptor_pool, layout);

  const vk::DescriptorImageInfo image_descriptor{
      .sampler = sampler,
      .imageView = image_view,

      .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
  };

  const vk::WriteDescriptorSet descriptor_write{
      .dstSet = descriptor_set,
      .dstBinding = 0,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = vk::DescriptorType::eCombinedImageSampler,
      .pImageInfo = &image_descriptor};

  logical_device.updateDescriptorSets(descriptor_write, nullptr);
}

void CubeMap::use(vk::CommandBuffer recording_command_buffer,
                  vk::PipelineLayout pipeline_layout) {
  recording_command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                              pipeline_layout, 1,
                                              descriptor_set, nullptr);
}
}  // namespace ice_image
