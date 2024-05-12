#include "config.hpp"

#ifndef ICE_WINDOW_HPP
#define ICE_WINDOW_HPP

#include <GLFW/glfw3.h>

namespace ice {

// C-Style API
inline void create_surface(VkInstance instance, GLFWwindow *window,
                           const VkAllocationCallbacks *allocator,
                           VkSurfaceKHR *surface) {
  // VkInstance, GLFW window pointer, custom allocator and pointer to
  // VkSurfaceKHR
  if (glfwCreateWindowSurface(instance, window, nullptr, surface) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create window surface!");
  }
}

struct Window2D {
  int width{0};
  int height{0};
};
// @brief Abstracts windowing functionalities.
class IceWindow {
public:
  IceWindow(int width, int height, std::string name);
  ~IceWindow();

  GLFWwindow *get_window() const { return window; }
  bool should_close() { return glfwWindowShouldClose(window); }

  void poll_events() { glfwPollEvents(); }

  double get_time() { return glfwGetTime(); }

  void set_window_title(std::string title) {
    glfwSetWindowTitle(window, title.data());
  }

  std::vector<const char *> get_required_extensions() const;

  Window2D get_framebuffer_size() const;
  void wait_events() const;

  int width;
  int height;

private:
  std::string window_name;
  GLFWwindow *window;
};

} // namespace ice

#endif