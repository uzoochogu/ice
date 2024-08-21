#ifndef VULKAN_ICE
#define VULKAN_ICE

#include "config.hpp"
#include "mesh.hpp"
#include "queue.hpp"

#include "camera.hpp"
#include "commands.hpp"
#include "descriptors.hpp"
#include "framebuffer.hpp"
#include "images/ice_cube_map.hpp"
#include "images/ice_texture.hpp"
#include "mesh_collator.hpp"
#include "multithreading/ice_jobs.hpp"
#include "multithreading/ice_worker_threads.hpp"
#include "pipeline.hpp"
#include "swapchain.hpp"
#include "synchronization.hpp"
#include "triangle_mesh.hpp"
#include "windowing.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

namespace ice {

// Abstraction over Vulkan initialization
class VulkanIce {
public:
#ifdef NDEBUG
  const bool enable_validation_layers = false;
#else
  const bool enable_validation_layers = true;
  void make_debug_messenger();

  // C-API
  static VKAPI_ATTR VkBool32 VKAPI_CALL
  debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                void *pUserData) {
    std::cout << "\nFrom Debug Callback!!!!\nvalidation layer: \n"
              << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
  }

  // debug callback
  vk::DebugUtilsMessengerEXT debug_messenger{nullptr};
#endif

  VulkanIce(IceWindow &window);
  ~VulkanIce();

  void render(Scene *scene);
  void setup_imgui_overlay();

  vk::PhysicalDevice get_physical_device() const { return physical_device; };

private:
  // instance setup
  void make_instance();

  // device setup
  void make_device();
  void setup_pipeline_bundles();
  void setup_swapchain(vk::SwapchainKHR *old_swapchain = nullptr);
  void recreate_swapchain();

  void setup_descriptor_set_layouts();

  // final frame resource setup
  void setup_framebuffers();
  void setup_command_buffers();
  void setup_frame_resources();

  // asset setup
  void make_worker_threads();
  void make_assets();
  void end_worker_threads();

  // frame and scene prep
  void prepare_frame(std::uint32_t image_index, Scene *scene);
  void prepare_scene(vk::CommandBuffer command_buffer);
  void record_sky_draw_commands(vk::CommandBuffer command_buffer,
                                uint32_t image_index, Scene *scene);
  void record_scene_draw_commands(vk::CommandBuffer command_buffer,
                                  uint32_t image_index, Scene *scene);
  void render_mesh(vk::CommandBuffer command_buffer, MeshTypes mesh_type,
                   uint32_t &start_instance, uint32_t instance_count);

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
  std::uint32_t max_frames_in_flight{0}, current_frame_index{0};

  // assets pointers
  // TriangleMesh *triangle_mesh;
  // std::unique_ptr<MeshCollator> meshes;
  MeshCollator *meshes;
  std::unordered_map<MeshTypes, ice_image::Texture *> materials;
  GltfMesh *gltf_mesh;
  ice_image::CubeMap *cube_map;
  Camera camera;

  // Job System
  bool done = false;
  ice_threading::WorkQueue work_queue;
  std::vector<std::jthread> workers;

  // descriptor-related variables
  std::unordered_map<PipelineType, vk::DescriptorSetLayout> frame_set_layout;
  vk::DescriptorPool frame_descriptor_pool;
  DescriptorSetLayoutData frame_set_layout_bindings;
  std::unordered_map<PipelineType, vk::DescriptorSetLayout> mesh_set_layout;
  vk::DescriptorPool mesh_descriptor_pool;
  DescriptorSetLayoutData mesh_set_layout_bindings;
  vk::DescriptorPool imgui_descriptor_pool;

  // command related variables
  vk::CommandPool command_pool;
  vk::CommandBuffer main_command_buffer;
  vk::CommandPool imgui_command_pool;

  // pipeline-related variables
  std::vector<PipelineType> pipeline_types = {PipelineType::SKY,
                                              PipelineType::STANDARD};
  std::unordered_map<PipelineType, vk::PipelineLayout> pipeline_layout;
  std::unordered_map<PipelineType, vk::RenderPass> renderpass;
  std::unordered_map<PipelineType, vk::Pipeline> pipeline;
  vk::RenderPass imgui_renderpass;

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
