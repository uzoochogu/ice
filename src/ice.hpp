#ifndef ICE_HPP
#define ICE_HPP

#include "vulkan_ice.hpp"
#include "windowing.hpp"

namespace ice {
class Ice {
public:
  Ice() = default;
  ~Ice();
  void run();
  void calculate_frame_rate();

private:
  double last_time, current_time;
  int num_frames;
  float frame_time;
  Scene scene;

  static constexpr int width{800};
  static constexpr int height{600};
  IceWindow window{width, height, "Ice engine!"};
  VulkanIce vulkan_backend{window};
};
} // namespace ice

#endif