#include "ice.hpp"
#include <sstream>

namespace ice {

Ice::~Ice() {}

// @brief Calculates the frame rate, sets the window title with this value
void Ice::calculate_frame_rate() {
  current_time = window.get_time();
  double delta = current_time - last_time;

  if (delta >= 1) {
    int framerate{std::max(1, int(num_frames / delta))};
    std::stringstream title;
    title << "Ice engine! Running at " << framerate << " fps.";
    window.set_window_title(title.str());
    last_time = current_time;
    num_frames = -1;
    frame_time = float(1000.0 / framerate);
  }

  ++num_frames;
}
void Ice::run() {

  while (!window.should_close()) {
    window.poll_events();
    vulkan_backend.render(&scene);
    calculate_frame_rate();
  }
}
} // namespace ice