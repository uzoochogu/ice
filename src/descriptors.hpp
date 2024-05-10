#ifndef DESCRIPTORS
#define DESCRIPTORS
#include "config.hpp"

namespace ice {

/**
        Describes the bindings of a descriptor set layout
*/
struct DescriptorSetLayoutData {
  std::uint32_t count;
  std::vector<std::uint32_t> indices;
  std::vector<vk::DescriptorType> types;
  std::vector<std::uint32_t> descriptor_counts;
  std::vector<vk::ShaderStageFlags> stages;
};

/**
        Make a descriptor set layout from the given descriptions

        \param device the logical device
        \param bindings	a struct describing the bindings used in the shader
        \returns the created descriptor set layout
*/
inline vk::DescriptorSetLayout
make_descriptor_set_layout(vk::Device device,
                           const DescriptorSetLayoutData &bindings) {

  /*
          Bindings describes a whole bunch of descriptor types, collect them all
     into a list of some kind.
  */
  std::vector<vk::DescriptorSetLayoutBinding> layout_bindings;
  layout_bindings.reserve(bindings.count);

  for (std::uint32_t i = 0; i < bindings.count; i++) {

    vk::DescriptorSetLayoutBinding layout_binding{
        .binding = bindings.indices[i],
        .descriptorType = bindings.types[i],
        .descriptorCount = bindings.descriptor_counts[i],
        .stageFlags = bindings.stages[i]};

    layout_bindings.push_back(layout_binding);
  }

  vk::DescriptorSetLayoutCreateInfo layout_info{
      .flags = vk::DescriptorSetLayoutCreateFlagBits(),
      .bindingCount = bindings.count,
      .pBindings = layout_bindings.data()};

  try {
    return device.createDescriptorSetLayout(layout_info);
  } catch (vk::SystemError err) {
#ifndef NDEBUG
    std::cout << "Failed to create Descriptor Set Layout\n";
#endif
    return nullptr;
  }
}

/**
        Make a descriptor pool

        \param device the logical device
        \param size the number of descriptor sets to allocate from the pool
        \param bindings	used to get the descriptor types
        \returns the created descriptor pool
*/
inline vk::DescriptorPool
make_descriptor_pool(vk::Device device, uint32_t size,
                     const DescriptorSetLayoutData &bindings) {

  std::vector<vk::DescriptorPoolSize> pool_sizes;

  for (std::uint32_t i = 0; i < bindings.count; i++) {
    vk::DescriptorPoolSize pool_size{.type = bindings.types[i],
                                     .descriptorCount = size

    };
    pool_sizes.push_back(pool_size);
  }

  vk::DescriptorPoolCreateInfo pool_info{
      .flags = vk::DescriptorPoolCreateFlags(),
      .maxSets = size,
      .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
      .pPoolSizes = pool_sizes.data()};

  try {
    return device.createDescriptorPool(pool_info);
  } catch (vk::SystemError err) {

#ifndef NDEBUG
    std::cout << "Failed to make descriptor pool\n";
#endif

    return nullptr;
  }
}

/**
        Allocate a descriptor set from a pool.

        \param device the logical device
        \param descriptorPool the pool to allocate from
        \param layout the descriptor set layout which the set must adhere to
        \returns the allocated descriptor set
*/

inline vk::DescriptorSet
allocate_descriptor_sets(vk::Device device, vk::DescriptorPool descriptor_pool,
                         vk::DescriptorSetLayout layout) {

  vk::DescriptorSetAllocateInfo allocation_info{.descriptorPool =
                                                    descriptor_pool,
                                                .descriptorSetCount = 1,
                                                .pSetLayouts = &layout};

  try {
    return device.allocateDescriptorSets(allocation_info)[0];
  } catch (vk::SystemError err) {
#ifndef NDEBUG
    std::cout << "Failed to allocate descriptor set from pool\n";
#endif
    return nullptr;
  }
}
} // namespace ice
#endif