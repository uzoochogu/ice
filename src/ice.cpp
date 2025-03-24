#include "ice.hpp"

#include <imgui.h>

#include <array>
#include <sstream>

#include "ui_compatibility.hpp"

namespace ice {

// Calculates the frame rate, sets the window title with this value
void Ice::calculate_frame_rate() {
  current_time = ice::IceWindow::get_time();
  const double delta = current_time - last_time;

  if (delta >= 1) {
    const int framerate{std::max(1, static_cast<int>(num_frames / delta))};
    std::stringstream title;
    title << "Ice engine! Running at " << framerate << " fps.";
    window.set_window_title(title.str());
    last_time = current_time;
    num_frames = -1;
    frame_time = static_cast<float>(1000.0 / framerate);
  }

  ++num_frames;
}

void Ice::apply_imgui_theme() {
  ImGui::StyleColorsDark();
  // ImGui::StyleColorsLight();

  // from https://github.com/ocornut/imgui/issues/707#issuecomment-1372640066 by
  // Trippasc
  ImVec4 *colors = ImGui::GetStyle().Colors;
  colors[ImGuiCol_WindowBg] = ImVec4{0.1f, 0.1f, 0.13f, .90f};
  colors[ImGuiCol_MenuBarBg] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};

  // Border
  colors[ImGuiCol_Border] = ImVec4{0.44f, 0.37f, 0.61f, 0.29f};
  colors[ImGuiCol_BorderShadow] = ImVec4{0.0f, 0.0f, 0.0f, 0.24f};

  // Text
  colors[ImGuiCol_Text] = ImVec4{1.0f, 1.0f, 1.0f, 1.0f};
  colors[ImGuiCol_TextDisabled] = ImVec4{0.5f, 0.5f, 0.5f, 1.0f};

  // Headers
  colors[ImGuiCol_Header] = ImVec4{0.13f, 0.13f, 0.17f, 1.0f};
  colors[ImGuiCol_HeaderHovered] = ImVec4{0.19f, 0.2f, 0.25f, 1.0f};
  colors[ImGuiCol_HeaderActive] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};

  // Buttons
  colors[ImGuiCol_Button] = ImVec4{0.13f, 0.13f, 0.17f, 1.0f};
  colors[ImGuiCol_ButtonHovered] = ImVec4{0.19f, 0.2f, 0.25f, 1.0f};
  colors[ImGuiCol_ButtonActive] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};
  colors[ImGuiCol_CheckMark] = ImVec4{0.74f, 0.58f, 0.98f, 1.0f};

  // Popups
  colors[ImGuiCol_PopupBg] = ImVec4{0.1f, 0.1f, 0.13f, 0.92f};

  // Slider
  colors[ImGuiCol_SliderGrab] = ImVec4{0.44f, 0.37f, 0.61f, 0.54f};
  colors[ImGuiCol_SliderGrabActive] = ImVec4{0.74f, 0.58f, 0.98f, 0.54f};

  // Frame BG
  colors[ImGuiCol_FrameBg] = ImVec4{0.13f, 0.13f, 0.17f, 1.0f};
  colors[ImGuiCol_FrameBgHovered] = ImVec4{0.19f, 0.2f, 0.25f, 1.0f};
  colors[ImGuiCol_FrameBgActive] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};

  // Tabs
  colors[ImGuiCol_Tab] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};
  colors[ImGuiCol_TabHovered] = ImVec4{0.24f, 0.24f, 0.32f, 1.0f};
  colors[ImGuiCol_TabActive] = ImVec4{0.2f, 0.22f, 0.27f, 1.0f};
  colors[ImGuiCol_TabUnfocused] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};

  // Title
  colors[ImGuiCol_TitleBg] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};
  colors[ImGuiCol_TitleBgActive] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};

  // Scrollbar
  colors[ImGuiCol_ScrollbarBg] = ImVec4{0.1f, 0.1f, 0.13f, 1.0f};
  colors[ImGuiCol_ScrollbarGrab] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4{0.19f, 0.2f, 0.25f, 1.0f};
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4{0.24f, 0.24f, 0.32f, 1.0f};

  // Seperator
  colors[ImGuiCol_Separator] = ImVec4{0.44f, 0.37f, 0.61f, 1.0f};
  colors[ImGuiCol_SeparatorHovered] = ImVec4{0.74f, 0.58f, 0.98f, 1.0f};
  colors[ImGuiCol_SeparatorActive] = ImVec4{0.84f, 0.58f, 1.0f, 1.0f};

  // Resize Grip
  colors[ImGuiCol_ResizeGrip] = ImVec4{0.44f, 0.37f, 0.61f, 0.29f};
  colors[ImGuiCol_ResizeGripHovered] = ImVec4{0.74f, 0.58f, 0.98f, 0.29f};
  colors[ImGuiCol_ResizeGripActive] = ImVec4{0.84f, 0.58f, 1.0f, 0.29f};

  // Docking
  // colors[ImGuiCol_DockingPreview] = ImVec4{0.44f, 0.37f, 0.61f, 1.0f};

  auto &style = ImGui::GetStyle();
  style.TabRounding = 4;
  style.ScrollbarRounding = 9;
  style.WindowRounding = 7;
  style.GrabRounding = 3;
  style.FrameRounding = 3;
  style.PopupRounding = 4;
  style.ChildRounding = 4;
}

void Ice::run() {
  vulkan_backend.setup_imgui_overlay();

  apply_imgui_theme();

  // imgui states
  bool show_demo_window = false;
  bool render_points = false;

  bool render_wireframe = vulkan_backend.render_wireframe;

  auto msaa_options = std::to_array<const char *>(
      {"1x - (Not ideal)", "2x", "4x", "8x", "16x"});
  static int msaa_current = 3;  // Default to 8x

  auto cull_options =
      std::to_array<const char *>({"None", "Front", "Back", "Front & Back"});
  static int cull_current = 2;  // Default to Back

  bool show_skybox = true;
  bool skybox = vulkan_backend.show_skybox;

  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

  // // Pipeline config struct
  // struct PipelineConfig{
  //   // Render points
  //   // Primitive type?
  //   // Render mode
  //   //
  // } config{};

  while (!window.should_close()) {
    ice::IceWindow::poll_events();

    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);

    {
      ImGui::SetNextWindowCollapsed(
          true,
          ImGuiCond_::ImGuiCond_Once);  // set next window collapsed state.

      ImGui::Begin("Show Info");

      ImGui::Text(
          "Frame Info:\nFrame rate = %.1f\nAverage Frame Time =  %.3f "
          "ms/frame ",
          io.Framerate, 1000.0f / io.Framerate);

      ImGui::Text("Graphics Card:\n%s", vulkan_backend.get_physical_device()
                                            .getProperties()
                                            .deviceName.data());

      ImGui::Checkbox("Demo Window", &show_demo_window);

      if (ImGui::Checkbox("Render Points", &render_points)) {
        vulkan_backend.render_points = render_points;
        vulkan_backend.rebuild_pipelines();
      }
      if (ImGui::Checkbox("Wireframe Mode", &render_wireframe)) {
        vulkan_backend.render_wireframe = render_wireframe;
        vulkan_backend.rebuild_pipelines();
      }

      if (ImGui::Combo("MSAA Samples", &msaa_current, msaa_options.data(),
                       msaa_options.size())) {
        msaa_current =
            ui_compatibility::set_msaa_samples(vulkan_backend, msaa_current);
      }

      if (ImGui::Combo("Culling Mode", &cull_current, cull_options.data(),
                       cull_options.size())) {
        cull_current =
            ui_compatibility::set_cull_mode(vulkan_backend, cull_current);
      }

      if (ImGui::Checkbox("Show Skybox", &skybox)) {
        // vulkan_backend.show_skybox = skybox;
        vulkan_backend.toggle_skybox(skybox);
      }

      if (vulkan_backend.render_wireframe) {
        float width = vulkan_backend.line_width;
        if (ImGui::SliderFloat("Line Width", &width, 1.0f, 10.0f)) {
          vulkan_backend.set_line_width(width);
        }
      }

      ImGui::End();
    }

    // Rendering
    ImGui::Render();

    vulkan_backend.render(&scene);
    calculate_frame_rate();
  }
}
}  // namespace ice
