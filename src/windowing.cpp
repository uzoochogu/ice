#include "windowing.hpp"

namespace ice {
IceWindow::IceWindow(int width, int height, std::string name) {

  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  window = glfwCreateWindow(width, height, name.data(), nullptr, nullptr);

  glfwSetWindowUserPointer(window, this);
}

IceWindow::~IceWindow() {
  glfwDestroyWindow(window);
  glfwTerminate();
}

// Returns the required list of extension based on whether validation layers
// are enabled or not,
std::vector<const char *> IceWindow::get_required_extensions() const {
  std::uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<const char *> extensions(
      glfwExtensions,
      glfwExtensions +
          glfwExtensionCount); // std::vector<T>(InputIt first, InputIt last)
  return extensions;
}

Window2D IceWindow::get_framebuffer_size() const {
  Window2D dimensions;
  glfwGetFramebufferSize(window, &dimensions.width, &dimensions.height);
  return dimensions;
}

void IceWindow::wait_events() const { glfwWaitEvents(); }
} // namespace ice
