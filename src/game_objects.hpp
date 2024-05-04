#ifndef GAME_OBJECT_HPP
#define GAME_OBJECT_HPP

#include "config.hpp"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // use Vulkan 0.0 to 1.0 not OpenGL's -1
                                    // to 1.0
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES // To help with alignments
                                           // requirements
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace ice {
// Data representing a gameobject e.g a model
struct GameObject {
  glm::mat4 model;
};

/**
 * Scene
 * Procedurally generated Scene data
 */
class Scene {

public:
  Scene();
  std::vector<glm::vec3> triangle_positions;
  // std::vector<glm::vec3> square_positions;
};

} // namespace ice

#endif