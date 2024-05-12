#ifndef VULKAN_ICE
#define VULKAN_ICE

#include "config.hpp"
#include "queue.hpp"

#include "commands.hpp"
#include "descriptors.hpp"
#include "framebuffer.hpp"
#include "mesh_collator.hpp"
#include "pipeline.hpp"
#include "swapchain.hpp"
#include "synchronization.hpp"
#include "triangle_mesh.hpp"
#include "windowing.hpp"

namespace ice {

// Loads and Wraps around vkCreateDebugUtilsMessengerEXT, creating
// vkDebugUtilsMessengerEXT object
static VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) { // instance is passed since debug messenger is
                         // specific to it and its layers
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else { // return nullptr if it couldn't be loaded
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

static void
DestroyDebugUtilsMessengerEXT(VkInstance instance,
                              VkDebugUtilsMessengerEXT debugMessenger,
                              const VkAllocationCallbacks *pAllocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debugMessenger, pAllocator);
  }
}

// @brief Abstraction over Vulkan initialization
class VulkanIce {
public:
#ifdef NDEBUG
  const bool enable_validation_layers = false;
#else
  const bool enable_validation_layers = true;
  void make_debug_messenger();

  static VKAPI_ATTR VkBool32 VKAPI_CALL
  debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                void *pUserData) {
    std::cout << "From Debug Callback!!!!" << std::endl;
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
  }

  // debug callback
  vk::DebugUtilsMessengerEXT debug_messenger{nullptr};
  VkDebugUtilsMessengerEXT debug_messenger_c_api{nullptr};
#endif

  VulkanIce(IceWindow &window);
  ~VulkanIce();

  void render(Scene *scene);

private:
  // instance setup
  void make_instance();

  // device setup
  void make_device();
  void setup_swapchain(vk::SwapchainKHR *old_swapchain = nullptr);
  void recreate_swapchain();

  void setup_descriptor_set_layouts();

  // final frame resource setup
  void setup_framebuffers();
  void setup_command_buffers();
  void setup_frame_resources();

  // asset setup
  void make_assets();
  void render_mesh(vk::CommandBuffer command_buffer, MeshTypes mesh_type,
                   uint32_t &start_instance, uint32_t instance_count);

  // frame and scene prep
  void prepare_frame(std::uint32_t image_index, Scene *scene);
  void prepare_scene(vk::CommandBuffer command_buffer);
  void record_draw_commands(vk::CommandBuffer command_buffer,
                            uint32_t image_index, Scene *scene);

  // cleanup
  void destroy_swapchain_bundle(bool include_swapchain = true);

  // utility functions
  bool is_validation_supported();
  void pick_physical_device();
  bool check_device_extension_support(const vk::PhysicalDevice &device);
  bool is_device_suitable(const vk::PhysicalDevice &device);

  // useful data
  QueueFamilyIndices indices;

  // utilities for synchronization
  std::uint32_t max_frames_in_flight{0}, current_frame{0};

  // assets pointers
  // TriangleMesh *triangle_mesh;
  // std::unique_ptr<MeshCollator> meshes;
  MeshCollator *meshes;
  std::unordered_map<MeshTypes, ice_image::Texture *> materials;

  // descriptor-related variables
  vk::DescriptorSetLayout frame_set_layout;
  vk::DescriptorPool frame_descriptor_pool;
  DescriptorSetLayoutData frame_set_layout_bindings;
  vk::DescriptorSetLayout mesh_set_layout;
  vk::DescriptorPool mesh_descriptor_pool;
  DescriptorSetLayoutData mesh_set_layout_bindings;

  // command related variables
  vk::CommandPool command_pool;
  vk::CommandBuffer main_command_buffer;

  // pipeline-related variables
  vk::PipelineLayout pipeline_layout;
  vk::RenderPass renderpass;
  vk::Pipeline pipeline;

  // device-related
  vk::SwapchainKHR swapchain{nullptr};
  std::vector<SwapChainFrame> swapchain_frames;
  vk::Format swapchain_format;
  vk::Extent2D swapchain_extent;
  vk::Queue graphics_queue{nullptr}, present_queue{nullptr};
  vk::PhysicalDevice physical_device{nullptr};
  vk::Device device{nullptr};

  // instance-related handles
  vk::SurfaceKHR surface;
  IceWindow &window;
  vk::DispatchLoaderDynamic dldi; // dynamic instance dispatcher
  vk::Instance instance{nullptr};

  const std::vector<const char *> validation_layers = {
      "VK_LAYER_KHRONOS_validation"};
  const std::vector<const char *> device_extensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};
};
} // namespace ice

#endif