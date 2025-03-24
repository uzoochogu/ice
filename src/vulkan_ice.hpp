#ifndef VULKAN_ICE_HPP
#define VULKAN_ICE_HPP

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "camera.hpp"
#include "commands.hpp"
#include "config.hpp"
#include "descriptors.hpp"
#include "framebuffer.hpp"
#include "images/ice_cube_map.hpp"
#include "images/ice_texture.hpp"
#include "mesh.hpp"
#include "mesh_collator.hpp"
#include "multithreading/ice_jobs.hpp"
#include "multithreading/ice_worker_threads.hpp"
#include "pipeline.hpp"
#include "queue.hpp"
#include "swapchain.hpp"
#include "synchronization.hpp"
#include "windowing.hpp"

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
  // NOLINTBEGIN (readability-identifier-naming)
  static VKAPI_ATTR VkBool32 VKAPI_CALL
  debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                void *pUserData) {
    std::cout << "\nFrom Debug Callback!!!!\nvalidation layer: \n"
              << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
  }
  // NOLINTEND (readability-identifier-naming)

  // debug callback
  vk::DebugUtilsMessengerEXT debug_messenger{nullptr};
#endif

  explicit VulkanIce(IceWindow &window);
  ~VulkanIce() noexcept;

  void render(Scene *scene);
  void setup_imgui_overlay();

  [[nodiscard]] vk::PhysicalDevice get_physical_device() const {
    return physical_device;
  }

  // UI settable states with setters
  bool render_points = false;
  bool render_wireframe = false;
  bool show_skybox = true;
  float line_width = 1.0f;
  vk::CullModeFlagBits cull_mode = vk::CullModeFlagBits::eBack;
  void rebuild_pipelines();
  void set_msaa_samples(vk::SampleCountFlagBits samples);
  void set_cull_mode(vk::CullModeFlagBits mode);
  void toggle_skybox(bool button);
  void set_line_width(float width);

  vk::SampleCountFlagBits get_max_sample_count();  // for MSAA support

 private:
  // instance setup
  void make_instance();

  // device setup
  void make_device();
  void setup_pipeline_bundles();
  void setup_swapchain(vk::SwapchainKHR *old_swapchain = nullptr);
  void recreate_swapchain(bool recreate_pipelines = false);

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
                                uint32_t image_index);
  void record_scene_draw_commands(vk::CommandBuffer command_buffer,
                                  uint32_t image_index, Scene *scene);
  void render_mesh(vk::CommandBuffer command_buffer, MeshTypes mesh_type,
                   uint32_t &start_instance, uint32_t instance_count);

  // cleanup
  void destroy_swapchain_bundle(bool include_swapchain = true) noexcept;
  void destroy_imgui_resources() noexcept;

  // utility functions
  bool is_validation_supported();
  void pick_physical_device();
  bool check_device_extension_support(
      const vk::PhysicalDevice &physical_device);
  bool is_device_suitable(const vk::PhysicalDevice &physical_device);

  // useful data
  QueueFamilyIndices indices;

  // utilities for synchronization
  std::uint32_t max_frames_in_flight{0}, current_frame_index{0};

  // assets pointers
  std::unique_ptr<MeshCollator> meshes;
  std::unordered_map<MeshTypes, std::shared_ptr<ice_image::Texture>> materials;
  std::unique_ptr<GltfMesh> gltf_mesh;
  std::unique_ptr<ice_image::CubeMap> cube_map;
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
  vk::Format swapchain_format{};
  vk::Extent2D swapchain_extent;
  vk::SampleCountFlagBits msaa_samples{vk::SampleCountFlagBits::e1};
  vk::Queue graphics_queue{nullptr}, present_queue{nullptr};
  vk::PhysicalDevice physical_device{nullptr};
  vk::Device device{nullptr};

  // instance-related handles
  vk::SurfaceKHR surface;
  IceWindow &window;
  vk::DispatchLoaderDynamic dldi;  // dynamic instance dispatcher
  vk::Instance instance{nullptr};

  const std::vector<const char *> validation_layers = {
      "VK_LAYER_KHRONOS_validation"};
  const std::vector<const char *> device_extensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};
};
}  // namespace ice

#endif  // VULKAN_ICE_HPP
