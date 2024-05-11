#include "vulkan_ice.hpp"
#include "images/ice_image.hpp"

namespace ice {
VulkanIce::VulkanIce(IceWindow &window) : window{window} {
  make_instance();

  // create surface, connect to vulkan
  VkSurfaceKHR c_style_surface;
  create_surface(instance, window.get_window(), nullptr, &c_style_surface);
  surface = c_style_surface; // cast to C++ style

  make_device();

  setup_swapchain(nullptr);

  setup_descriptor_set_layouts();

  // Render pass and Pipeline creation
  GraphicsPipelineInBundle spec{
      .device = device,
      .vertex_filepath = "resources/shaders/vert.spv",
      .fragment_filepath = "resources/shaders/frag.spv",
      .swapchain_extent = swapchain_extent,
      .swapchain_image_format = swapchain_format,
      .swapchain_depth_format =
          swapchain_frames[0].depth_format /* at least 1 is guaranteed*/,

      .descriptor_set_layouts = {frame_set_layout, mesh_set_layout}};

  GraphicsPipelineOutBundle graphics_bundle = make_graphics_pipeline(spec);

  pipeline_layout = std::move(graphics_bundle.layout);
  renderpass = std::move(graphics_bundle.renderpass);
  pipeline = std::move(graphics_bundle.pipeline);

  // populate frame buffers
  setup_framebuffers();

  // command pool and command buffers
  setup_command_buffers();

  // Make synchronization objects
  setup_frame_resources();

  make_assets();
}

VulkanIce::~VulkanIce() {

  device.waitIdle();

  std::cout << "Vulkan Destroyed" << std::endl;

  device.destroyCommandPool(command_pool);

  device.destroyPipeline(pipeline);
  device.destroyPipelineLayout(pipeline_layout);
  device.destroyRenderPass(renderpass);

  destroy_swapchain_bundle();

  device.destroyDescriptorSetLayout(frame_set_layout);
  device.destroyDescriptorSetLayout(mesh_set_layout);
  device.destroyDescriptorPool(mesh_descriptor_pool);

  delete meshes;

  for (const auto &[key, texture] : materials) {
    delete texture;
  }

  device.destroy();

  instance.destroyDebugUtilsMessengerEXT(debug_messenger, nullptr, dldi);
  DestroyDebugUtilsMessengerEXT(instance, debug_messenger_c_api, nullptr);
  instance.destroySurfaceKHR(surface);
  instance.destroy();
}

void VulkanIce::make_instance() {
  vk::ApplicationInfo appInfo{.pApplicationName = "Ice App",
                              .applicationVersion =
                                  VK_MAKE_API_VERSION(0, 1, 0, 0),
                              .pEngineName = "Ice",
                              .engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
                              .apiVersion = VK_API_VERSION_1_0};

  auto extensions = window.get_required_extensions();

  if (enable_validation_layers) {
    extensions.push_back(
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME); // VK_EXT_debug_utils

    if (!is_validation_supported()) {
      std::cerr << "validation layers requested, but not available!"
                << std::endl;
      throw std::runtime_error(
          "validation layers requested, but not available!");
    }
  }

#ifndef NDEBUG
  std::cout << "\n\nWindowing Extensions: \n";
  for (auto extension : extensions) {
    std::cout << extension << std::endl;
  }

  std::cout << "\n\nLayers: \n";
  for (auto layer_name : validation_layers) {
    std::cout << layer_name << std::endl;
  }
#endif

  vk::InstanceCreateInfo createInfo{
      /* .flags = vk::InstanceCreateFlags(), */
      .pApplicationInfo = &appInfo,
      .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
      .ppEnabledExtensionNames = extensions.data()};

  if (enable_validation_layers) {
    createInfo.enabledLayerCount =
        static_cast<std::uint32_t>(validation_layers.size());
    createInfo.ppEnabledLayerNames = validation_layers.data();
  }

#ifndef NDEBUG
  for (int i = 0; i < validation_layers.size(); i++) {
    std::cout << std::format("Layer {} {}\n", i,
                             createInfo.ppEnabledLayerNames[i]);
  }
  std::cout << std::format("Layer count {}\n", createInfo.enabledLayerCount);
#endif

  try {
    instance = vk::createInstance(createInfo);
  } catch (vk::SystemError err) {
    std::cerr << "failed to create Instance!" << std::endl;
    std::runtime_error("Instance creation failed. Exiting...");
  }

  // init Instance Dynamic loader after instance is created
  dldi = vk::DispatchLoaderDynamic(instance, vkGetInstanceProcAddr);
  make_debug_messenger();
}

// Physical Device
void VulkanIce::make_device() {
  pick_physical_device();

  VulkanIce::indices = find_queue_families(physical_device, surface);

  // create Queues from the Queue Families
  std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;

#ifndef NDEBUG
  std::cout << std::format(
      "Graphics family value {}\n"
      "Present Family value : {}\n",
      indices.graphics_family.value_or(0xFFFF),
      indices.present_family.value_or(0xFFFF)); // prevent crash for debugging
#endif

  // Queue families, ensure uniqueness
  std::set<std::uint32_t> unique_queue_families = {
      indices.graphics_family.value(), indices.present_family.value()};

  float queue_priority = 1.0f;
  for (std::uint32_t queue_family : unique_queue_families) {
    vk::DeviceQueueCreateInfo queue_create_info{
        .queueFamilyIndex = queue_family,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority};

    queue_create_infos.push_back(queue_create_info);
  }

  // Pick device features you want
  vk::PhysicalDeviceFeatures device_features{};

  // Create device
  vk::DeviceCreateInfo device_info{
      .queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size()),
      .pQueueCreateInfos = queue_create_infos.data(),
      .enabledExtensionCount = static_cast<uint32_t>(device_extensions.size()),
      .ppEnabledExtensionNames = device_extensions.data(),
      .pEnabledFeatures = &device_features};

  // validation layers
  if (enable_validation_layers) {
    device_info.enabledLayerCount =
        static_cast<uint32_t>(validation_layers.size());
    device_info.ppEnabledLayerNames = validation_layers.data();
  }

  /* try {
    device = physical_device.createDevice(device_info, nullptr);
  } catch (vk::SystemError err) {
    std::cerr << "failed to create logical device" << std::endl;
    throw std::runtime_error("failed to create logical device");
  } */
  device = physical_device.createDevice(device_info, nullptr);
  if (device == nullptr) {
    std::cerr << "failed to create logical device" << std::endl;
    throw std::runtime_error("failed to create logical device");
  }

  // Pair Device with Queue
  VulkanIce::graphics_queue =
      device.getQueue(indices.graphics_family.value(), 0);
  VulkanIce::present_queue = device.getQueue(indices.present_family.value(), 0);
}

void VulkanIce::setup_swapchain(vk::SwapchainKHR *old_swapchain) {
  // swapchain creation
  SwapChainBundle bundle = create_swapchain_bundle(
      physical_device, device, surface, window.get_framebuffer_size().width,
      window.get_framebuffer_size().height, old_swapchain);
  swapchain = bundle.swapchain;
  swapchain_frames = bundle.frames;
  swapchain_format = bundle.format;
  swapchain_extent =
      bundle.frames[0].extent; // at least one frame is guaranteed

  // set max frames
  max_frames_in_flight = static_cast<uint32_t>(swapchain_frames.size());
}

void VulkanIce::setup_descriptor_set_layouts() {
  // Populate DescriptorSetLayoutData
  // UBO Binding 0
  frame_set_layout_bindings;

  frame_set_layout_bindings = {.count = 2};
  frame_set_layout_bindings.indices.push_back(0);
  frame_set_layout_bindings.types.push_back(vk::DescriptorType::eUniformBuffer);
  frame_set_layout_bindings.descriptor_counts.push_back(1);
  frame_set_layout_bindings.stages.push_back(vk::ShaderStageFlagBits::eVertex);

  // model transforms binding 1
  frame_set_layout_bindings.indices.push_back(1);
  frame_set_layout_bindings.types.push_back(vk::DescriptorType::eStorageBuffer);
  frame_set_layout_bindings.descriptor_counts.push_back(1);
  frame_set_layout_bindings.stages.push_back(vk::ShaderStageFlagBits::eVertex);

  // set and bindings once per frame
  frame_set_layout =
      make_descriptor_set_layout(device, frame_set_layout_bindings);

  // bindings for individual draw calls
  mesh_set_layout_bindings = {.count = 1};

  mesh_set_layout_bindings.indices.push_back(0);
  mesh_set_layout_bindings.types.push_back(
      vk::DescriptorType::eCombinedImageSampler);
  mesh_set_layout_bindings.descriptor_counts.push_back(1);
  mesh_set_layout_bindings.stages.push_back(vk::ShaderStageFlagBits::eFragment);

  mesh_set_layout =
      make_descriptor_set_layout(device, mesh_set_layout_bindings);
}

void VulkanIce::setup_framebuffers() {
  // Finalize setup
  FramebufferInput framebuffer_input{.device = device,
                                     .renderpass = renderpass,
                                     .swapchain_extent = swapchain_extent};

  // populate frame buffers
  make_framebuffers(framebuffer_input, swapchain_frames);
}

// Creates a command pool, the main command buffer and a command buffer for each
// frame
void VulkanIce::setup_command_buffers() {
  command_pool = make_command_pool(device, physical_device, surface);
  CommandBufferReq command_buffer_req = {.device = device,
                                         .command_pool = command_pool,
                                         .frames = swapchain_frames};
  main_command_buffer = make_command_buffer(command_buffer_req);
  make_frame_command_buffers(command_buffer_req);
}

// Sets up frame resources like sync objects, UBOs etc
void VulkanIce::setup_frame_resources() {
  // A descriptor set per frame
  frame_descriptor_pool = make_descriptor_pool(
      device, static_cast<uint32_t>(swapchain_frames.size()),
      frame_set_layout_bindings);

  for (SwapChainFrame &frame : swapchain_frames) {
    frame.in_flight_fence = make_fence(device);
    frame.image_available = make_semaphore(device);
    frame.render_finished = make_semaphore(device);

    frame.make_descriptor_resources();
    frame.descriptor_set = allocate_descriptor_sets(
        device, frame_descriptor_pool, frame_set_layout);
  }
}

void VulkanIce::recreate_swapchain() {
#ifndef NDEBUG
  std::cout << "Recreating swapchain\n";
#endif

  // To handle minimization (frame buffer size = 0)
  // pause until the window is in the foreground again
  // check for size
  ice::Window2D window_dim{window.get_framebuffer_size()};
  while (window_dim.width == 0 || window_dim.width == 0) {
    window_dim = window.get_framebuffer_size();
    window.wait_events();
  }
  device.waitIdle();

  // preserve old swapchain handle for recreation
  vk::SwapchainKHR old_swapchain = swapchain;
#ifndef NDEBUG
  std::cout << "Transfer operation started" << std::endl;
#endif

  destroy_swapchain_bundle(false);
#ifndef NDEBUG
  std::cout << "Destroy swapchain bundle" << std::endl;
#endif

  // redo everything related to a frame
  setup_swapchain(&old_swapchain);
#ifndef NDEBUG
  std::cout << "Setup new swapchain" << std::endl;
#endif

  device.destroySwapchainKHR(old_swapchain);
#ifndef NDEBUG
  std::cout << "Destroyed old swapchain" << std::endl;
#endif

  setup_framebuffers();
  setup_frame_resources();
  // just setup frame command buffers, since pool and main buffer is already
  // created
  CommandBufferReq command_buffer_req = {.device = device,
                                         .command_pool = command_pool,
                                         .frames = swapchain_frames};
  make_frame_command_buffers(command_buffer_req);
}

void VulkanIce::make_assets() {
  meshes = new MeshCollator();

  // <mesh type , filenames, pre_transforms>
  std::vector<std::tuple<MeshTypes, std::vector<const char *>, glm::mat4>>
      model_inputs = {
          {MeshTypes::GROUND,
           /* obj file path, material file path*/
           {"resources/models/ground.obj", "resources/models/ground.mtl"},
           glm::mat4(1.0f)},
          /* Rotate 180 degrees about the z axis*/
          {MeshTypes::GIRL,
           {"resources/models/girl.obj", "resources/models/girl.mtl"},
           glm::rotate(glm::mat4(1.0f), glm::radians(180.0f),
                       glm::vec3(0.0f, 0.0f, 1.0f))},
          {MeshTypes::SKULL,
           {"resources/models/skull.obj", "resources/models/skull.mtl"},
           glm::mat4(1.0f)}};

  // std::pair<MeshTypes, std::vector<const char*>, glm::mat4> pair
  for (const auto &[mesh_types, filenames, pre_transform] : model_inputs) {
    ObjMesh model(filenames[0], filenames[1], pre_transform);
    meshes->consume(mesh_types, model.vertices, model.indices);
  }

  VertexBufferFinalizationInput finalization_info{
      .logical_device = device,
      .physical_device = physical_device,
      .command_buffer = main_command_buffer,
      .queue = graphics_queue};

  meshes->finalize(finalization_info);

  // Materials (Textures)
  std::unordered_map<MeshTypes, const char *> filenames = {
      {MeshTypes::GROUND, "resources/textures/ground.jpg"},
      {MeshTypes::GIRL, "resources/textures/none.png"},
      {MeshTypes::SKULL, "resources/textures/skull.png"}};

  // mesh descriptor pool
  mesh_descriptor_pool =
      make_descriptor_pool(device, static_cast<uint32_t>(filenames.size()),
                           mesh_set_layout_bindings);

  ice_image::TextureCreationInput texture_info{
      .physical_device = physical_device,
      .logical_device = device,
      .command_buffer = main_command_buffer,
      .queue = graphics_queue,
      .layout = mesh_set_layout,
      .descriptor_pool = mesh_descriptor_pool};

  for (const auto &[object, filename] : filenames) {
    texture_info.filename = filename;
    materials[object] = new ice_image::Texture(texture_info);
  }
  std::cout << "Finished making assets" << std::endl;
}

void VulkanIce::prepare_frame(std::uint32_t image_index, Scene *scene) {

  /* x =0 and a height of 1*/
  glm::vec3 eye = {0.0f, 0.0f, 1.0f};
  // { forwards, , inline with us }
  glm::vec3 center = {1.0f, 0.0f, 1.0f};
  // { z is up  }
  glm::vec3 up = {0.0f, 0.0f, 1.0f};
  glm::mat4 view = glm::lookAt(eye, center, up);

  // increase far distance to prevent clipping
  glm::mat4 projection =
      glm::perspective(glm::radians(45.0f),
                       static_cast<float>(swapchain_extent.width) /
                           static_cast<float>(swapchain_extent.height),
                       0.1f, 100.0f);
  projection[1][1] *= -1;

  SwapChainFrame &_frame = swapchain_frames[image_index];
  _frame.camera_data = {.view = view,
                        .projection = projection,
                        .view_projection = projection * view};
  memcpy(_frame.camera_data_write_location, &(_frame.camera_data), sizeof(UBO));

  // model transforms info
  size_t i = 0;
  for (std::pair<MeshTypes, std::vector<glm::vec3>> pair : scene->positions) {
    for (glm::vec3 &position : pair.second) {
      _frame.model_transforms[i++] = glm::translate(glm::mat4(1.0f), position);
    }
  }

  memcpy(_frame.model_buffer_write_location, _frame.model_transforms.data(),
         i * sizeof(glm::mat4));

  _frame.write_descriptor_set();
}

void VulkanIce::prepare_scene(vk::CommandBuffer command_buffer) {
  vk::Buffer vertex_buffers[] = {meshes->vertex_buffer.buffer};
  vk::DeviceSize offsets[] = {0};
  command_buffer.bindVertexBuffers(0, 1, vertex_buffers, offsets);
  command_buffer.bindIndexBuffer(meshes->index_buffer.buffer, 0,
                                 vk::IndexType::eUint32);
}

// @brief Logic for rendering frames.
void VulkanIce::render(Scene *scene) {
  vk::Result result =
      device.waitForFences(1, &swapchain_frames[current_frame].in_flight_fence,
                           vk::True, UINT64_MAX);
  // reset fence just before queue submit
  result =
      device.resetFences(1, &swapchain_frames[current_frame].in_flight_fence);

  std::uint32_t image_index;

  // try-catch because exception would be throw by Vulkan-hpp once error is
  // detected.
  try {
    // acquireNextImageKHR(vk::SwapChainKHR, timeout, semaphore_to_signal,
    // fence)
    vk::ResultValue acquire = device.acquireNextImageKHR(
        swapchain, UINT64_MAX, swapchain_frames[current_frame].image_available,
        nullptr);
    image_index = acquire.value;
  } catch (vk::OutOfDateKHRError) {
#ifndef NDEBUG
    std::cout << "Acquire error, Out of DateKHR\n";
#endif
    recreate_swapchain();
    return;
  }

  vk::CommandBuffer command_buffer =
      swapchain_frames[current_frame]
          .command_buffer; // was swapchain_frames[image_index].comand_buffer

  command_buffer.reset();

  prepare_frame(image_index, scene);

  record_draw_commands(command_buffer, image_index, scene);

  vk::SubmitInfo submit_info = {};

  vk::Semaphore wait_semaphores[] = {
      swapchain_frames[current_frame].image_available};
  vk::PipelineStageFlags wait_stages[] = {
      vk::PipelineStageFlagBits::eColorAttachmentOutput};
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = wait_semaphores;
  submit_info.pWaitDstStageMask = wait_stages;

  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer;

  vk::Semaphore signal_semaphores[] = {
      swapchain_frames[current_frame].render_finished};
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = signal_semaphores;

  try {
    graphics_queue.submit(submit_info,
                          swapchain_frames[current_frame].in_flight_fence);
  } catch (vk::SystemError err) {

    throw std::runtime_error("failed to submit draw command buffer!");
  }

  vk::SwapchainKHR swapchains[] = {swapchain};
  vk::PresentInfoKHR present_info = {
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = signal_semaphores,

      .swapchainCount = 1,
      .pSwapchains = swapchains,

      .pImageIndices = &image_index,
  };

  // vk::Result present_result;
  try {
    result = present_queue.presentKHR(present_info);
  } catch (vk::OutOfDateKHRError) {
#ifndef NDEBUG
    std::cout << "Present Error, Out of DateKHR/ Suboptimal KHR\n";
#endif
    // result = vk::Result::eErrorOutOfDateKHR;
    if (result == vk::Result::eErrorOutOfDateKHR ||
        result == vk::Result::eSuboptimalKHR) {
      recreate_swapchain();
      return;
    }
  }

  // increment current frame, loop around e.g  (0->1->0)
  current_frame = (current_frame + 1) % max_frames_in_flight;
}

void VulkanIce::render_mesh(vk::CommandBuffer command_buffer,
                            MeshTypes mesh_type, uint32_t &start_instance,
                            uint32_t instance_count) {
  int index_count = meshes->index_counts.find(mesh_type)->second;
  int first_index = meshes->index_lump_offsets.find(mesh_type)->second;
  materials[mesh_type]->use(command_buffer, pipeline_layout);
  command_buffer.drawIndexed(index_count, instance_count, first_index, 0,
                             start_instance);
  start_instance += instance_count;
}

void VulkanIce::record_draw_commands(vk::CommandBuffer command_buffer,
                                     uint32_t image_index, Scene *scene) {

  vk::CommandBufferBeginInfo begin_info = {};

  try {
    command_buffer.begin(begin_info);
  } catch (vk::SystemError err) {
    throw std::runtime_error("Failed to begin recording command buffer!");
  }
  // clear values unions
  vk::ClearValue clear_color =
      vk::ClearColorValue({std::array<float, 4>{1.0f, 0.5f, 0.25f, 1.0f}});
  vk::ClearValue clear_depth = vk::ClearDepthStencilValue({1.0f, 0});

  std::vector<vk::ClearValue> clear_values = {{clear_color, clear_depth}};

  vk::RenderPassBeginInfo renderpass_info = {
      .renderPass = renderpass,
      .framebuffer = swapchain_frames[image_index].framebuffer,
      .renderArea{
          .offset{.x = 0, .y = 0},
          .extent = swapchain_extent,
      },

      .clearValueCount = static_cast<std::uint32_t>(clear_values.size()),
      .pClearValues = clear_values.data(),
  };

  command_buffer.beginRenderPass(&renderpass_info,
                                 vk::SubpassContents::eInline);

  command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

  command_buffer.bindDescriptorSets(
      vk::PipelineBindPoint::eGraphics, pipeline_layout, 0,
      swapchain_frames[image_index].descriptor_set, nullptr);

  prepare_scene(command_buffer);

  // instancing
  // Triangles
  std::uint32_t start_instance = 0;

  for (const auto &[mesh_types, positions] : scene->positions) {
    render_mesh(command_buffer, mesh_types, start_instance,
                static_cast<uint32_t>(positions.size()));
  }

  command_buffer.endRenderPass();

  try {
    command_buffer.end();
  } catch (vk::SystemError err) {

    throw std::runtime_error("failed to record command buffer!");
  }
}

// Debug Messenger
#ifndef NDEBUG
void VulkanIce::make_debug_messenger() {
  if (!enable_validation_layers) {
    return;
  }
  std::cout << "Making debug messenger\n\n" << std::endl;
  // fill structure with details about the messenger and its callback
  vk::DebugUtilsMessengerCreateInfoEXT createInfo = {
      /* .flags = vk::DebugUtilsMessengerCreateFlagsEXT(), */
      .messageSeverity =
          /* exclude Info bit */
      /*       vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | */
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
      .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                     vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                     vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
      .pfnUserCallback = VulkanIce::debugCallback};

  std::cout << std::format("Message Severity: {}\nMessage Type: {}\n",
                           vk::to_string(createInfo.messageSeverity),
                           vk::to_string(createInfo.messageType))
            << std::endl;
  debug_messenger =
      instance.createDebugUtilsMessengerEXT(createInfo, nullptr, dldi);

  if (debug_messenger == nullptr) {
    throw std::runtime_error("failed to set up debug messenger!");
  }

  // created Debug messenger with C-API
  VkDebugUtilsMessengerCreateInfoEXT debugInfo{
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,

      .messageSeverity =
          /* exclude Info bit */
      /*       vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | */
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
      .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      .pfnUserCallback = VulkanIce::debugCallback,
      .pUserData = nullptr};
  CreateDebugUtilsMessengerEXT(instance, &debugInfo, nullptr,
                               &debug_messenger_c_api);
}
#endif

// Utils
bool VulkanIce::is_validation_supported() {
  std::uint32_t layer_count;
  vk::Result err = vk::enumerateInstanceLayerProperties(
      &layer_count,
      nullptr); // vkEnumerateXxxxYyyyProperties
                // queries Xxxx properties

  std::vector<vk::LayerProperties> available_layers(layer_count);
  err = vk::enumerateInstanceLayerProperties(&layer_count,
                                             available_layers.data());

  if (err != vk::Result::eSuccess) {
    std::cerr << "Problem enumerating validation layers properties"
              << std::endl;
    return false;
  }

#ifndef NDEBUG
  std::cout << "\n\nAvailable Layers: \n";
  for (auto layer_name : available_layers) {
    std::cout << layer_name.layerName << std::endl;
  }
#endif

  // Check if all requested validation layers exist in available layers
  std::set<std::string> required_layers(validation_layers.begin(),
                                        validation_layers.end());
  for (const auto &layer_properties : available_layers) {
    required_layers.erase(layer_properties.layerName);
  }

  // If any layer is missing, return false
  return required_layers.empty();
}

bool VulkanIce::check_device_extension_support(
    const vk::PhysicalDevice &device) {

  std::vector<vk::ExtensionProperties> available_extensions =
      device.enumerateDeviceExtensionProperties();

  // Copy required extensions defined globally
  std::set<std::string> required_extensions(device_extensions.begin(),
                                            device_extensions.end());
  for (const auto &extension : available_extensions) {
    required_extensions.erase(
        extension.extensionName); // erase will succeed if present
  }
  return required_extensions.empty(); // all required extensions are present
}

// internal utility functions
bool VulkanIce::is_device_suitable(const vk::PhysicalDevice &device) {
  QueueFamilyIndices indices = find_queue_families(device, surface);
  bool extensions_supported = check_device_extension_support(device);

  // TODO query for swapchain support  if extensions supported

  if (!extensions_supported) {
    return false;
  }

  // TODO return true if indices.isComplete() && extensions_supported &&
  // swapChainAdequate &&
  // supportedFeatures.samplerAnisotropy

  std::cout << std::format("The value of indices: {}\n",
                           indices.graphics_family.has_value());
  // return indices.graphics_family.has_value();
  return indices.is_complete();
}

// \todo declare callable argument for picking strategy
// \brief Picks a physical device using a strategy - default strategy is the
// first suitable device.
// \exception It throws a runtime error if it fails to pick a physical device.
void VulkanIce::pick_physical_device() {
  std::vector<vk::PhysicalDevice> available_devices =
      instance.enumeratePhysicalDevices();

  for (auto device : available_devices) {
    // algorithm : Pick the first suitable device
    if (is_device_suitable(device)) {
      VulkanIce::physical_device = device;

#ifndef NDEBUG
      // log the picked device properties

      vk::PhysicalDeviceProperties properties = physical_device.getProperties();

      std::cout << std::format("Device Name: {}\n"
                               "Device Type: {}\n",
                               /* Formatter not available??*/
                               std::string(properties.deviceName.begin(),
                                           properties.deviceName.end()),
                               vk::to_string(properties.deviceType));
#endif
      return;
    }
  }

#ifndef NDEBUG
  std::cerr << "failed to pick a physical device!" << std::endl;
#endif

  throw std::runtime_error("failed to pick a physical device!");
}

void VulkanIce::destroy_swapchain_bundle(bool include_swapchain) {
  for (auto frame : swapchain_frames) {
    frame.destroy();
  }

  if (include_swapchain) {
    device.destroySwapchainKHR(swapchain);
  }

  device.destroyDescriptorPool(frame_descriptor_pool);
}

} // namespace ice