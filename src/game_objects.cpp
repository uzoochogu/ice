#include "game_objects.hpp"

namespace ice {

Scene::Scene() {

  float x = -0.3f;
  for (float z = -1.0f; z <= 1.0f; z += 0.2f) {
    for (float y = -1.0f; y <= 1.0f; y += 0.2f) {

      triangle_positions.push_back(glm::vec3(x, y, z));
    }
  }

  x = 0.0f;
  for (float z = -1.0f; z <= 1.0f; z += 0.2f) {
    for (float y = -1.0f; y < 1.0f; y += 0.2f) {

      square_positions.push_back(glm::vec3(x, y, z));
    }
  }

  x = 0.3f;
  for (float z = -1.0f; z <= 1.0f; z += 0.2f) {
    for (float y = -1.0f; y < 1.0f; y += 0.2f) {

      star_positions.push_back(glm::vec3(x, y, z));
    }
  }
}
} // namespace ice