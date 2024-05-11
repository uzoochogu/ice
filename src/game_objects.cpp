#include "game_objects.hpp"

namespace ice {

Scene::Scene() {

  positions.insert({MeshTypes::GROUND, {}});
  positions.insert({MeshTypes::GIRL, {}});
  positions.insert({MeshTypes::SKULL, {}});

  // One ground and girl, 2 skulls

  // model transorm
  // glm::vec3 {distance away from cam, left/right of cam, height above cam
  // level}

  // Ground is pushed forward in front of the camera, dead centre with the cam,
  // and on the ground i.e height of zero
  positions[MeshTypes::GROUND].push_back(glm::vec3(10.0f, 0.0f, 0.0f));
  positions[MeshTypes::GIRL].push_back(glm::vec3(17.0f, 0.0f, 0.0f));
  positions[MeshTypes::SKULL].push_back(glm::vec3(15.0f, -5.0f, 1.0f));
  positions[MeshTypes::SKULL].push_back(glm::vec3(15.0f, 5.0f, 1.0f));
}
} // namespace ice