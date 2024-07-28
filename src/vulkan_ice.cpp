#include "vulkan_ice.hpp"
#include "game_objects.hpp"
#include "mesh.hpp"

namespace ice {
VulkanIce::VulkanIce(IceWindow &window)
    : window{window}, camera{800, 600, glm::vec3(6.5f, -6.5f, 5.0f)} {
  make_instance();

  // create surface, connect to vulkan
  VkSurfaceKHR c_style_surface;
  create_surface(instance, window.get_window(), nullptr, &c_style_surface);
  surface = c_style_surface; // cast to C++ style

  // ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  make_device();

  setup_swapchain(nullptr);

  setup_descriptor_set_layouts();

  // Render pass and Pipeline creation
  setup_pipeline_bundles();

  // populate frame buffers
  setup_framebuffers();

  // command pool and command buffers
  setup_command_buffers();

  // Make synchronization objects
  setup_frame_resources();

  make_worker_threads();
  make_assets();
  end_worker_threads();
}

VulkanIce::~VulkanIce() {

  device.waitIdle();

#ifndef NDEBUG
  std::cout << "Vulkan Destroyed" << std::endl;
#endif

  device.destroyCommandPool(command_pool);

  for (PipelineType pipeline_type : pipeline_types) {
    device.destroyPipeline(pipeline[pipeline_type]);
    device.destroyPipelineLayout(pipeline_layout[pipeline_type]);
    device.destroyRenderPass(renderpass[pipeline_type]);
  }

  destroy_swapchain_bundle();

  for (PipelineType pipeline_type : pipeline_types) {
    device.destroyDescriptorSetLayout(frame_set_layout[pipeline_type]);
    device.destroyDescriptorSetLayout(mesh_set_layout[pipeline_type]);
  }

  device.destroyDescriptorPool(mesh_descriptor_pool);

  // imgui resource cleanup
  device.destroyRenderPass(imgui_renderpass);
  device.destroyCommandPool(imgui_command_pool);
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  device.destroyDescriptorPool(imgui_descriptor_pool);
  ImGui::DestroyContext();

  delete meshes;
  delete gltf_mesh;

  for (const auto &[key, texture] : materials) {
    delete texture;
  }

  delete cube_map;

  device.destroy();

#ifndef NDEBUG
  instance.destroyDebugUtilsMessengerEXT(debug_messenger, nullptr, dldi);
#endif
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

#ifndef NDEBUG
  make_debug_messenger();
#endif
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

  // update camera dims
  camera.set_width(window.get_framebuffer_size().width);
  camera.set_height(window.get_framebuffer_size().height);

  // set max frames
  max_frames_in_flight = static_cast<uint32_t>(swapchain_frames.size());
}

void VulkanIce::setup_descriptor_set_layouts() {
  // Populate DescriptorSetLayoutData
  // UBO Binding 0
  frame_set_layout_bindings = {.count = 1};
  frame_set_layout_bindings.indices.push_back(0);
  frame_set_layout_bindings.types.push_back(vk::DescriptorType::eUniformBuffer);
  frame_set_layout_bindings.descriptor_counts.push_back(1);
  frame_set_layout_bindings.stages.push_back(vk::ShaderStageFlagBits::eVertex);

  // SKY PIPELINE needs only 1 uniform binding
  frame_set_layout[PipelineType::SKY] =
      make_descriptor_set_layout(device, frame_set_layout_bindings);

  // model transforms binding 1
  frame_set_layout_bindings.count = 2;
  frame_set_layout_bindings.indices.push_back(1);
  frame_set_layout_bindings.types.push_back(vk::DescriptorType::eStorageBuffer);
  frame_set_layout_bindings.descriptor_counts.push_back(1);
  frame_set_layout_bindings.stages.push_back(vk::ShaderStageFlagBits::eVertex);

  // set and bindings once per frame for STANDARD PIPELINE
  frame_set_layout[PipelineType::STANDARD] =
      make_descriptor_set_layout(device, frame_set_layout_bindings);

  // bindings for individual draw calls
  mesh_set_layout_bindings = {.count = 1};

  mesh_set_layout_bindings.indices.push_back(0);
  mesh_set_layout_bindings.types.push_back(
      vk::DescriptorType::eCombinedImageSampler);
  mesh_set_layout_bindings.descriptor_counts.push_back(1);
  mesh_set_layout_bindings.stages.push_back(vk::ShaderStageFlagBits::eFragment);

  mesh_set_layout[PipelineType::SKY] =
      make_descriptor_set_layout(device, mesh_set_layout_bindings);
  mesh_set_layout[PipelineType::STANDARD] =
      make_descriptor_set_layout(device, mesh_set_layout_bindings);
}

namespace {
// Utility Error Function used for ImGui code.
static void check_vk_result(VkResult err) {
  if (err == 0)
    return;
#ifndef NDEBUG
  fprintf(stderr, "ImGui [vulkan] Error: VkResult = %d\n", err);
#endif
  if (err < 0)
    abort();
}
} // namespace

void VulkanIce::setup_imgui_overlay() {
  // Platform/Renderer backends
  ImGui_ImplGlfw_InitForVulkan(window.get_window(), true);
  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = instance;
  init_info.PhysicalDevice = physical_device;
  init_info.Device = device;
  init_info.QueueFamily = indices.graphics_family.value();
  init_info.Queue = graphics_queue;
  init_info.PipelineCache = vk::PipelineCache{};
  init_info.DescriptorPool = imgui_descriptor_pool;
  init_info.RenderPass = imgui_renderpass;
  init_info.Subpass = 0;
  {
    // Already sized to the minImageCount + 1 &&  < MaxImageCount
    std::uint32_t image_count = static_cast<uint32_t>(swapchain_frames.size());
    init_info.MinImageCount = image_count;
    init_info.ImageCount = image_count;
  }
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  init_info.Allocator = nullptr;
  init_info.CheckVkResultFn = check_vk_result;
  ImGui_ImplVulkan_Init(&init_info);
}

void VulkanIce::setup_pipeline_bundles() {
  // SKY PIPELINE
  // load and store op is don't care, will present.
  GraphicsPipelineInBundle spec{
      .device = device,
      .vertex_filepath = "resources/shaders/sky_vert.spv",
      .fragment_filepath = "resources/shaders/sky_frag.spv",
      .swapchain_extent = swapchain_extent,
      .swapchain_image_format = {swapchain_format},
      .descriptor_set_layouts = {frame_set_layout[PipelineType::SKY],
                                 mesh_set_layout[PipelineType::SKY]},
      .load_op = vk::AttachmentLoadOp::eDontCare,
      .initial_layout = vk::ImageLayout::eUndefined};

  GraphicsPipelineOutBundle graphics_bundle = make_graphics_pipeline(spec);

  pipeline_layout[PipelineType::SKY] = std::move(graphics_bundle.layout);
  renderpass[PipelineType::SKY] = std::move(graphics_bundle.renderpass);
  pipeline[PipelineType::SKY] = std::move(graphics_bundle.pipeline);

  // STANDARD PIPELINE
  spec.vertex_filepath = "resources/shaders/vert.spv";
  spec.fragment_filepath = "resources/shaders/frag.spv";
  spec.attribute_descriptions = Vertex::get_attribute_descriptions();
  spec.binding_description = Vertex::get_binding_description();
  spec.swapchain_depth_format = {
      swapchain_frames[0].depth_format} /* at least 1 is guaranteed*/;
  spec.descriptor_set_layouts = {frame_set_layout[PipelineType::STANDARD],
                                 mesh_set_layout[PipelineType::STANDARD]};
  spec.load_op = vk::AttachmentLoadOp::eLoad;
  spec.initial_layout = vk::ImageLayout::ePresentSrcKHR;

  graphics_bundle = make_graphics_pipeline(spec);

  pipeline_layout[PipelineType::STANDARD] = std::move(graphics_bundle.layout);
  renderpass[PipelineType::STANDARD] = std::move(graphics_bundle.renderpass);
  pipeline[PipelineType::STANDARD] = std::move(graphics_bundle.pipeline);

  // imgui renderpass
  imgui_renderpass = make_imgui_renderpass(device, swapchain_format,
                                           swapchain_frames[0].depth_format);
}

void VulkanIce::setup_framebuffers() {
  // Finalize setup
  FramebufferInput framebuffer_input{.device = device,
                                     .renderpass = renderpass,
                                     .imgui_renderpass = imgui_renderpass,
                                     .swapchain_extent = swapchain_extent};

  // populate frame buffers (including imgui's), one for each swapchain image.
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

  // imgui command buffers
  imgui_command_pool = make_command_pool(device, physical_device, surface);
  command_buffer_req.command_pool = imgui_command_pool;

  // frame imgui command buffers
  command_buffer_req.command_pool = imgui_command_pool;
  make_imgui_command_buffers(command_buffer_req);
}

// Sets up frame resources like sync objects, UBOs etc
void VulkanIce::setup_frame_resources() {
  const std::uint32_t descriptor_set_per_frame = 2;
  frame_descriptor_pool = make_descriptor_pool(
      device,
      descriptor_set_per_frame * static_cast<uint32_t>(swapchain_frames.size()),
      frame_set_layout_bindings);

  for (SwapChainFrame &frame : swapchain_frames) {
    frame.in_flight_fence = make_fence(device);
    frame.image_available = make_semaphore(device);
    frame.render_finished = make_semaphore(device);

    frame.make_descriptor_resources();
    frame.descriptor_sets[PipelineType::SKY] = allocate_descriptor_sets(
        device, frame_descriptor_pool, frame_set_layout[PipelineType::SKY]);
    frame.descriptor_sets[PipelineType::STANDARD] =
        allocate_descriptor_sets(device, frame_descriptor_pool,
                                 frame_set_layout[PipelineType::STANDARD]);

    frame.record_write_operations();
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

  // recreate Imgui frame command buffers
  command_buffer_req.command_pool = imgui_command_pool;
  make_imgui_command_buffers(command_buffer_req);
}

void VulkanIce::make_worker_threads() {
  done = false;
  size_t thread_count = std::thread::hardware_concurrency() - 1;

  workers.reserve(thread_count);
  CommandBufferReq command_buffer_input = {device, command_pool,
                                           swapchain_frames};
  for (size_t i = 0; i < thread_count; ++i) {
    vk::CommandBuffer command_buffer =
        make_command_buffer(command_buffer_input);
    workers.push_back(std::thread(ice_threading::WorkerThread(
        work_queue, done, command_buffer, graphics_queue)));
  }
}

void VulkanIce::make_assets() {

  // Meshes
  meshes = new MeshCollator();

  // <mesh type , filenames, pre_transforms> prep
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

  /*   //  pair
    for (const auto &[mesh_types, filenames, pre_transform] : model_inputs) {
      ObjMesh model(filenames[0], filenames[1], pre_transform);
      meshes->consume(mesh_types, model.vertices, model.indices);
    } */

  // stores models to be loaded
  std::unordered_map<MeshTypes, ObjMesh> models;

  // Materials (Textures) prep
  std::unordered_map<MeshTypes, const char *> texture_filenames = {
      {MeshTypes::GROUND, "resources/textures/ground.jpg"},
      {MeshTypes::GIRL, "resources/textures/none.png"},
      {MeshTypes::SKULL, "resources/textures/skull.png"}};

  // mesh descriptor pool to allocate sets
  mesh_descriptor_pool = make_descriptor_pool(
      device, static_cast<uint32_t>(texture_filenames.size()) + 1,
      mesh_set_layout_bindings); // extra set for cube map

  ice_image::TextureCreationInput texture_info{
      .physical_device = physical_device,
      .logical_device = device,
      .command_buffer = main_command_buffer,
      .queue = graphics_queue,
      .layout = mesh_set_layout[PipelineType::STANDARD],
      .descriptor_pool = mesh_descriptor_pool};

  // Submit loading work
  work_queue.lock.lock();
  // std::tuple<MeshTypes, std::vector<const char*>, glm::mat4>
  for (const auto &[mesh_type, obj_mtl_filename, pre_transform] :
       model_inputs) {
    texture_info.filenames = {texture_filenames[mesh_type]};

    // Default construct without loading
    materials[mesh_type] = new ice_image::Texture();
    models[mesh_type] = ObjMesh();

    // Add loading jobs
    work_queue.add(
        new ice_threading::MakeTexture(materials[mesh_type], texture_info));
    // MakeModel(ice::ObjMesh &mesh, const char *obj_filepath, const char
    // *mtl_filepath, glm::mat4 pre_transform)
    work_queue.add(
        new ice_threading::MakeModel(models[mesh_type], obj_mtl_filename[0],
                                     obj_mtl_filename[1], pre_transform));
  }

  work_queue.lock.unlock();

  // loading work to be done by background thread, wait till completion
#ifndef NDEBUG
  std::cout << "Waiting for work to finish." << std::endl;
#endif

  while (true) {
    if (!work_queue.lock.try_lock()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      continue;
    }

    if (work_queue.done()) {
#ifndef NDEBUG
      std::cout << "Work finished" << std::endl;
#endif
      work_queue.clear();
      work_queue.lock.unlock();
      break;
    }
    work_queue.lock.unlock();
  }

  // Consume loaded meshes
  // std::pair<MeshTypes, ObjMesh>
  for (const auto &[mesh_type, model] : models) {
    meshes->consume(mesh_type, model.vertices, model.indices);
  }

  VertexBufferFinalizationInput finalization_info{
      .logical_device = device,
      .physical_device = physical_device,
      .command_buffer = main_command_buffer,
      .queue = graphics_queue};

  meshes->finalize(finalization_info);

  // Sky Texture
  texture_info.layout = mesh_set_layout[PipelineType::SKY];
  texture_info.filenames = {{
      "resources/textures/sky_front.png",  // x+
      "resources/textures/sky_back.png",   // x-
      "resources/textures/sky_left.png",   // y+
      "resources/textures/sky_right.png",  // y-
      "resources/textures/sky_bottom.png", // z+
      "resources/textures/sky_top.png",    // z-
  }};
  cube_map = new ice_image::CubeMap(texture_info);

  // GltfMesh
  // Create a larger descriptor pool for GLTF mesh textures
  DescriptorSetLayoutData gltf_texture_layout_bindings = {
      .count = 1,
      .indices = {0},
      .types = {vk::DescriptorType::eCombinedImageSampler},
      .descriptor_counts = {1},
      .stages = {vk::ShaderStageFlagBits::eFragment}};

  vk::DescriptorPool gltf_descriptor_pool =
      make_descriptor_pool(device, 1000, gltf_texture_layout_bindings);

  // TRS
  glm::mat4 pre_transform =
      glm::translate(glm::mat4(1.0f), glm::vec3(15.0f, 3.0f, 5.0f));

  pre_transform = glm::rotate(pre_transform, glm::radians(120.0f),
                              glm::vec3(1.0f, 1.0f, 1.0f));
  pre_transform = glm::rotate(pre_transform, glm::radians(180.0f),
                              glm::vec3(0.0f, 1.0f, 0.0f));

  pre_transform = glm::scale(pre_transform, glm::vec3(3.0f, 3.0f, 3.0f));

  // make GLTF MESH
  gltf_mesh = new GltfMesh(
      physical_device, device, main_command_buffer, graphics_queue,
      mesh_set_layout[PipelineType::STANDARD], gltf_descriptor_pool,
      // "resources/models/Box.gltf", pre_transform);
      // "resources/models/ToyCar.glb", pre_transform); // very tiny increase
      // scale to see it
      // "resources/models/Suzanne.gltf", pre_transform);
      "resources/models/DamagedHelmet.gltf", pre_transform);

  std::array<vk::DescriptorPoolSize, 11> imgui_pool_sizes = {
      vk::DescriptorPoolSize{vk::DescriptorType::eSampler, 1000},
      vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, 1000},
      vk::DescriptorPoolSize{vk::DescriptorType::eSampledImage, 1000},
      vk::DescriptorPoolSize{vk::DescriptorType::eStorageImage, 1000},
      vk::DescriptorPoolSize{vk::DescriptorType::eUniformTexelBuffer, 1000},
      vk::DescriptorPoolSize{vk::DescriptorType::eStorageTexelBuffer, 1000},
      vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, 1000},
      vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, 1000},
      vk::DescriptorPoolSize{vk::DescriptorType::eUniformBufferDynamic, 1000},
      vk::DescriptorPoolSize{vk::DescriptorType::eStorageBufferDynamic, 1000},
      vk::DescriptorPoolSize{vk::DescriptorType::eInputAttachment, 1000}};

  vk::DescriptorPoolCreateInfo imgui_pool_info{
      .flags = vk::DescriptorPoolCreateFlags() |
               vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
      .maxSets = 1,
      .poolSizeCount = static_cast<uint32_t>(imgui_pool_sizes.size()),
      .pPoolSizes = imgui_pool_sizes.data()};

  try {
    imgui_descriptor_pool = device.createDescriptorPool(imgui_pool_info);
  } catch (vk::SystemError err) {
#ifndef NDEBUG
    std::cout << "Failed to make imgui descriptor pool\n";
#endif
  }
#ifndef NDEBUG
  std::cout << "Finished making assets" << std::endl;
#endif
}

void VulkanIce::end_worker_threads() {
  done = true;

  for (size_t i = 0; i < workers.size(); ++i) {
    workers[i].join();
  }
#ifndef NDEBUG
  std::cout << "Threads ended successfully." << std::endl;
#endif
}

void VulkanIce::prepare_frame(std::uint32_t image_index, Scene *scene) {

  // camera code
  camera.inputs(&window);
  // increase far plane distance to prevent clipping
  camera.update_matrices(45.0f, 0.1f, 100000.0f);

  SwapChainFrame &_frame =
      swapchain_frames[image_index]; // swapchain frame alias

  // update camera vector
  _frame.camera_vector_data = camera.get_camera_vector();

  memcpy(_frame.camera_vector_write_location, &(_frame.camera_vector_data),
         sizeof(CameraVectors));

  // update camera matrix
  _frame.camera_matrix_data = camera.get_camera_matrix();

  memcpy(_frame.camera_matrix_write_location, &(_frame.camera_matrix_data),
         sizeof(CameraMatrices));

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
  const ice::SwapChainFrame &current_frame =
      swapchain_frames[current_frame_index]; // alias

  vk::Result result = device.waitForFences(1, &current_frame.in_flight_fence,
                                           vk::True, UINT64_MAX);
  // reset fence just before queue submit
  result = device.resetFences(1, &current_frame.in_flight_fence);

  std::uint32_t acquired_image_index;

  // try-catch because exception would be throw by Vulkan-hpp once error is
  // detected.
  try {
    // acquireNextImageKHR(vk::SwapChainKHR, timeout, semaphore_to_signal,
    // fence)
    vk::ResultValue acquire = device.acquireNextImageKHR(
        swapchain, UINT64_MAX, current_frame.image_available, nullptr);
    acquired_image_index = acquire.value;
  } catch (vk::OutOfDateKHRError) {
#ifndef NDEBUG
    std::cout << "Acquire error, Out of DateKHR\n";
#endif
    recreate_swapchain();
    return;
  }

  vk::CommandBuffer command_buffer = current_frame.command_buffer;

  command_buffer.reset();

  prepare_frame(acquired_image_index, scene);

  // begin recording
  vk::CommandBufferBeginInfo begin_info = {};

  try {
    command_buffer.begin(begin_info);
  } catch (vk::SystemError err) {
    throw std::runtime_error("Failed to begin recording command buffer!");
  }
  // record events
  record_sky_draw_commands(command_buffer, acquired_image_index, scene);
  record_scene_draw_commands(command_buffer, acquired_image_index, scene);

  // end
  try {
    command_buffer.end();
  } catch (vk::SystemError err) {

    throw std::runtime_error("failed to record command buffer!");
  }

  // Recording ImGui Command Buffers
  {
    begin_info = {.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
    current_frame.imgui_command_buffer.begin(begin_info);
    // Render pass
    {
      // clear values unions
      vk::ClearValue clear_color =
          vk::ClearColorValue({std::array<float, 4>{1.0f, 0.5f, 0.25f, 1.0f}});

      vk::RenderPassBeginInfo info = {
          .renderPass = imgui_renderpass,
          .framebuffer =
              swapchain_frames[acquired_image_index].imgui_framebuffer,
          .renderArea = {.extent = {.width = swapchain_extent.width,
                                    .height = swapchain_extent.height}},
          .clearValueCount = 1,
          .pClearValues = &clear_color,
      };
      current_frame.imgui_command_buffer.beginRenderPass(
          info, vk::SubpassContents::eInline);
    }

    // ImGui Render command
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),
                                    current_frame.imgui_command_buffer);
    // Submit command buffer
    current_frame.imgui_command_buffer.endRenderPass();
    current_frame.imgui_command_buffer.end();
  }

  // submit all command buffers
  std::array<vk::CommandBuffer, 2> submit_command_buffers = {
      command_buffer, current_frame.imgui_command_buffer};

  // semaphores
  vk::Semaphore wait_semaphores[] = {current_frame.image_available};
  vk::PipelineStageFlags wait_stages[] = {
      vk::PipelineStageFlagBits::eColorAttachmentOutput};
  vk::Semaphore signal_semaphores[] = {current_frame.render_finished};

  vk::SubmitInfo submit_info = {
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = wait_semaphores,
      .pWaitDstStageMask = wait_stages,
      .commandBufferCount =
          static_cast<uint32_t>(submit_command_buffers.size()),
      .pCommandBuffers = submit_command_buffers.data(),

      .signalSemaphoreCount = 1,
      .pSignalSemaphores = signal_semaphores};

  try {
    graphics_queue.submit(submit_info, current_frame.in_flight_fence);
  } catch (vk::SystemError err) {

    throw std::runtime_error("failed to submit draw command buffer!");
  }

  vk::SwapchainKHR swapchains[] = {swapchain};
  vk::PresentInfoKHR present_info = {
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = signal_semaphores,
      .swapchainCount = 1,
      .pSwapchains = swapchains,
      .pImageIndices = &acquired_image_index,
  };

  try {
    result = present_queue.presentKHR(present_info);
  } catch (vk::OutOfDateKHRError) {
#ifndef NDEBUG
    std::cout << "Present Error, Out of DateKHR/ Suboptimal KHR\n";
#endif
    if (result == vk::Result::eErrorOutOfDateKHR ||
        result == vk::Result::eSuboptimalKHR) {
      recreate_swapchain();
      return;
    }
  }

  // increment current frame, loop around e.g  (0->1->0)
  current_frame_index = (current_frame_index + 1) % max_frames_in_flight;
}

void VulkanIce::render_mesh(vk::CommandBuffer command_buffer,
                            MeshTypes mesh_type, uint32_t &start_instance,
                            uint32_t instance_count) {
  int index_count = meshes->index_counts.find(mesh_type)->second;
  int first_index = meshes->index_lump_offsets.find(mesh_type)->second;
  materials[mesh_type]->use(command_buffer,
                            pipeline_layout[PipelineType::STANDARD]);
  command_buffer.drawIndexed(index_count, instance_count, first_index, 0,
                             start_instance);
  start_instance += instance_count;
}

void VulkanIce::record_sky_draw_commands(vk::CommandBuffer command_buffer,
                                         uint32_t image_index, Scene *scene) {
  // clear values unions
  vk::ClearValue clear_color =
      vk::ClearColorValue({std::array<float, 4>{1.0f, 0.5f, 0.25f, 1.0f}});
  vk::ClearValue clear_depth = vk::ClearDepthStencilValue({1.0f, 0});

  std::vector<vk::ClearValue> clear_values = {{clear_color, clear_depth}};

  vk::RenderPassBeginInfo renderpass_info = {
      .renderPass = renderpass[PipelineType::SKY],
      .framebuffer =
          swapchain_frames[image_index].framebuffer[PipelineType::SKY],
      .renderArea{
          .offset{.x = 0, .y = 0},
          .extent = swapchain_extent,
      },

      .clearValueCount = static_cast<std::uint32_t>(clear_values.size()),
      .pClearValues = clear_values.data(),
  };

  command_buffer.beginRenderPass(&renderpass_info,
                                 vk::SubpassContents::eInline);

  command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                              pipeline[PipelineType::SKY]);

  // dynamic viewport and scissor specification
  vk::Viewport viewport{.x = 0.0f,
                        .y = 0.0f,
                        .width = static_cast<float>(swapchain_extent.width),
                        .height = static_cast<float>(swapchain_extent.height),
                        .minDepth = 0.0f,
                        .maxDepth = 1.0f};
  command_buffer.setViewport(0, 1, &viewport);

  vk::Rect2D scissor{.offset = {0, 0}, .extent = swapchain_extent};
  command_buffer.setScissor(0, 1, &scissor);

  command_buffer.bindDescriptorSets(
      vk::PipelineBindPoint::eGraphics, pipeline_layout[PipelineType::SKY], 0,
      swapchain_frames[image_index].descriptor_sets[PipelineType::SKY],
      nullptr);

  cube_map->use(command_buffer, pipeline_layout[PipelineType::SKY]);
  command_buffer.draw(6, 1, 0, 0);

  command_buffer.endRenderPass();
}

void VulkanIce::record_scene_draw_commands(vk::CommandBuffer command_buffer,
                                           uint32_t image_index, Scene *scene) {
  // clear values unions
  vk::ClearValue clear_color =
      vk::ClearColorValue({std::array<float, 4>{1.0f, 0.5f, 0.25f, 1.0f}});
  vk::ClearValue clear_depth = vk::ClearDepthStencilValue({1.0f, 0});

  std::vector<vk::ClearValue> clear_values = {{clear_color, clear_depth}};

  vk::RenderPassBeginInfo renderpass_info = {
      .renderPass = renderpass[PipelineType::STANDARD],
      .framebuffer =
          swapchain_frames[image_index].framebuffer[PipelineType::STANDARD],
      .renderArea{
          .offset{.x = 0, .y = 0},
          .extent = swapchain_extent,
      },

      .clearValueCount = static_cast<std::uint32_t>(clear_values.size()),
      .pClearValues = clear_values.data(),
  };

  command_buffer.beginRenderPass(&renderpass_info,
                                 vk::SubpassContents::eInline);

  command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                              pipeline[PipelineType::STANDARD]);

  vk::Viewport viewport{.x = 0.0f,
                        .y = 0.0f,
                        .width = static_cast<float>(swapchain_extent.width),
                        .height = static_cast<float>(swapchain_extent.height),
                        .minDepth = 0.0f,
                        .maxDepth = 1.0f};
  command_buffer.setViewport(0, 1, &viewport);

  vk::Rect2D scissor{.offset = {0, 0}, .extent = swapchain_extent};
  command_buffer.setScissor(0, 1, &scissor);

  command_buffer.bindDescriptorSets(
      vk::PipelineBindPoint::eGraphics, pipeline_layout[PipelineType::STANDARD],
      0, swapchain_frames[image_index].descriptor_sets[PipelineType::STANDARD],
      nullptr);

  prepare_scene(command_buffer);

  // instancing
  // Triangles
  std::uint32_t start_instance = 0;

  for (const auto &[mesh_types, positions] : scene->positions) {
    render_mesh(command_buffer, mesh_types, start_instance,
                static_cast<uint32_t>(positions.size()));
  }

  // Bind GltfMesh buffers and draw
  for (size_t i = 0; i < gltf_mesh->mesh_buffers.size(); ++i) {
    const auto &mesh_buffer = gltf_mesh->mesh_buffers[i];

    // Bind vertex buffer
    vk::Buffer vertex_buffers[] = {mesh_buffer.vertex_buffer.buffer};
    vk::DeviceSize offsets[] = {0};
    command_buffer.bindVertexBuffers(0, 1, vertex_buffers, offsets);

    // Bind index buffer
    command_buffer.bindIndexBuffer(mesh_buffer.index_buffer.buffer, 0,
                                   vk::IndexType::eUint32);

    // Bind texture
    if (i < gltf_mesh->textures.size() && gltf_mesh->textures[i] != nullptr) {
      gltf_mesh->textures[i]->use(command_buffer,
                                  pipeline_layout[PipelineType::STANDARD]);
    }

    // Draw the mesh
    uint32_t index_count = gltf_mesh->index_counts[i];
    command_buffer.drawIndexed(index_count, 1, 0, 0, 0);
  }

  command_buffer.endRenderPass();
}

// Debug Messenger
#ifndef NDEBUG
void VulkanIce::make_debug_messenger() {
  if (!enable_validation_layers) {
    return;
  }
  std::cout << "Making debug messenger\n\n" << std::endl;
  // fill structure with details about the messenger and its callback
  vk::DebugUtilsMessengerCreateInfoEXT create_info = {
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
                           vk::to_string(create_info.messageSeverity),
                           vk::to_string(create_info.messageType))
            << std::endl;
  debug_messenger =
      instance.createDebugUtilsMessengerEXT(create_info, nullptr, dldi);

  if (debug_messenger == nullptr) {
    throw std::runtime_error("failed to set up debug messenger!");
  }
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

#ifndef NDEBUG
  std::cout << std::format("The value of indices: {}\n",
                           indices.graphics_family.has_value());
#endif
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
                               properties.deviceName.data(),
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
    frame.destroy(imgui_command_pool);
  }

  if (include_swapchain) {
    device.destroySwapchainKHR(swapchain);
  }

  device.destroyDescriptorPool(frame_descriptor_pool);
}

} // namespace ice
