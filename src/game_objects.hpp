#ifndef GAME_OBJECT_HPP
#define GAME_OBJECT_HPP

#include "./config.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE  // use Vulkan 0.0 to 1.0 not OpenGL's -1
                                     // to 1.0
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES  // To help with alignments
                                            // requirements
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

#ifndef NDEBUG
#include <glm/gtx/string_cast.hpp>
#endif
namespace ice {
// Data representing a game object e.g a model
struct GameObject {
  glm::mat4 model;
};

//--------- Assets -------------//
enum class MeshTypes { GROUND, GIRL, SKULL };

/**
 * Scene
 * Procedurally generated Scene data
 */
class Scene {
 public:
  Scene();

  std::unordered_map<MeshTypes, std::vector<glm::vec3>> positions{};
};

}  // namespace ice

#endif  // GAME_OBJECT_HPP
