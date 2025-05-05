#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <cmath>

#include "game_objects.hpp"
#include "windowing.hpp"

namespace ice {
// Camera data

// Transformation matrix
struct CameraMatrices {
  glm::mat4 view;
  glm::mat4 projection;
  glm::mat4 view_projection;
};

// Camera direction
struct CameraVectors {
  glm::vec4 forwards;
  glm::vec4 right;
  glm::vec4 up;
  float tanHalfFovY;
  float tanHalfFovX;
};

struct CameraDimensions {
  std::uint32_t width{800};
  std::uint32_t height{600};
};

class Camera {
 public:
  Camera(CameraDimensions dim, glm::vec3 position);

  [[nodiscard]] CameraMatrices get_camera_matrix() const;
  [[nodiscard]] CameraVectors get_camera_vector() const;

  void update_matrices(float FOV_deg, float near_plane, float far_plane);

  // Handles keyboard and mouse inputs
  void inputs(IceWindow *ice_window);

  // window
  void set_width(const std::uint32_t new_width) { width = new_width; }
  void set_height(const std::uint32_t new_height) { height = new_height; }

 private:
  CameraMatrices camera_matrix;
  CameraVectors camera_vector{
      .forwards = glm::vec4({0.0f, 0.0f, 1.0f, 0.0f}),
      .right = glm::vec4({1.0f, 0.0f, 0.0f, 0.0f}),
      .up = glm::vec4({0.0f, 1.0f, 0.0f, 0.0f}),

      .tanHalfFovY = std::tanf(45.0 * 0.5),
      .tanHalfFovX =
          std::tanf(45.0 * 0.5) *
          static_cast<float>(800.0 /
                             600.0)};  // using 45 degree and 800/600 as default

  // Stores the main vectors of the camera
  glm::vec3 position;
  const glm::vec3 DEFAULT_POSITION;
  glm::vec3 orientation = glm::vec3(0.0f, 0.0f, 1.0f);  // Forward along +Z
  glm::vec3 up =
      glm::vec3(0.0f, 1.0f, 0.0f);  // Up along -Y (Note Vulkan Y is down)

  // Prevents the camera from jumping around when first clicking left click
  bool first_click{true}, camera_active{false};

  // window
  std::uint32_t width;
  std::uint32_t height;

  // Adjust the speed of the camera and it's sensitivity when looking around
  float default_speed{0.005f}, speed = {0.005f}, sensitivity{5.0f};
};

}  // namespace ice

#endif  // CAMERA_HPP
