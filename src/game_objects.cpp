#include "game_objects.hpp"

namespace ice {

Scene::Scene() {
  positions.insert({MeshTypes::GROUND, {}});
  positions.insert({MeshTypes::GIRL, {}});
  positions.insert({MeshTypes::SKULL, {}});

  // One ground and girl, 2 skulls model transforms

  // Coordinate system from GLM (OpenGL) Left handed from Model's perspective
  // Camera's perspective: right is (-x), up is (+y),
  // forward into screen is (+z),
  positions[MeshTypes::GROUND].emplace_back(0.0f, 0.0f, 0.0f);
  positions[MeshTypes::GIRL].emplace_back(0.0f, 0.0f, 0.0f);
  positions[MeshTypes::SKULL].emplace_back(-5.0f, 3.0f, 1.0f);
  positions[MeshTypes::SKULL].emplace_back(5.0f, 3.0f, 1.0f);
}
}  // namespace ice
