#ifndef CAMERA_HPP
#define CAMERA_HPP

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
};

class Camera {
public:
  Camera(int width, int height, glm::vec3 position);

  CameraMatrices get_camera_matrix() const;
  CameraVectors get_camera_vector() const;

  void update_matrices(float FOV_deg, float near_plane, float far_plane);

  // Handles keyboard and mouse inputs
  void inputs(IceWindow *ice_window);

  // window
  void set_width(std::uint32_t width) { width = width; }
  void set_height(std::uint32_t height) { height = height; };

private:
  CameraMatrices camera_matrix;
  CameraVectors camera_vector{glm::vec4({1.0f, 0.0f, 0.0f, 0.0f}),
                              glm::vec4({0.0f, -1.0f, 0.0f, 0.0f}),
                              glm::vec4({0.0f, 0.0f, 1.0f, 0.0f})};
  // Stores the main vectors of the camera
  glm::vec3 position;
  const glm::vec3 DEFAULT_POSITION;
  glm::vec3 orientation = glm::vec3(2.0f, 0.0f, 0.0f);
  glm::vec3 up = {0.0f, 0.0f, 1.0f};

  // Prevents the camera from jumping around when first clicking left click
  bool first_click = true;

  // window
  std::uint32_t width;
  std::uint32_t height;

  // Adjust the speed of the camera and it's sensitivity when looking around
  float speed = 0.005f;
  float sensitivity = 5.0f;
};

} // namespace ice
#endif
