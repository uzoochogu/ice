#include "game_objects.hpp"

namespace ice {

Scene::Scene() {
  positions.insert({MeshTypes::GROUND, {}});
  positions.insert({MeshTypes::GIRL, {}});
  positions.insert({MeshTypes::SKULL, {}});

  // One ground and girl, 2 skulls

  // model transform
  // glm::vec3 {distance away from cam, left/right of cam, height above cam
  // level}

  // Ground is pushed forward in front of the camera, dead centre with the cam,
  // and on the ground i.e height of zero
  positions[MeshTypes::GROUND].emplace_back(10.0f, 0.0f, 0.0f);
  positions[MeshTypes::GIRL].emplace_back(17.0f, 0.0f, 0.0f);
  positions[MeshTypes::SKULL].emplace_back(15.0f, -5.0f, 1.0f);
  positions[MeshTypes::SKULL].emplace_back(15.0f, 5.0f, 1.0f);
}
}  // namespace ice
