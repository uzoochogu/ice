#include "game_objects.hpp"

namespace ice {

Scene::Scene() {

  for (float x = -1.0f; x < 1.0f; x += 0.2f) {
    // float x = -0.6f;
    for (float y = -1.0f; y < 1.0f; y += 0.2f) {

      triangle_positions.push_back(glm::vec3(x, y, 0.0f));
    }

    /*   x = 0.0f;
      for (float y = -1.0f; y < 1.0f; y += 0.2f) {

        square_positions.push_back(glm::vec3(x, y, 0.0f));
      } */
  }
}
} // namespace ice