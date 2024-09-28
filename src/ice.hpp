#ifndef ICE_HPP
#define ICE_HPP

#include "vulkan_ice.hpp"
#include "windowing.hpp"

namespace ice {
class Ice {
 public:
  Ice() = default;
  ~Ice() = default;
  void run();
  void calculate_frame_rate();

 private:
  static void apply_imgui_theme();
  double last_time{}, current_time{};
  int num_frames{};
  float frame_time{};
  Scene scene;

  IceWindow window{800, 600, "Ice engine!"};
  VulkanIce vulkan_backend{window};
};
}  // namespace ice

#endif  // ICE_HPP
