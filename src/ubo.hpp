#ifndef UBO_HPP
#define UBO_HPP

#include "game_objects.hpp"

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

} // namespace ice
#endif