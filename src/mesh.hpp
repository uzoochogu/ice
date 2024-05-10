#ifndef MESH_HPP
#define MESH_HPP

#include "game_objects.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp> // for hashing glm::vec data structures

namespace ice {

// Vertex data
// To be used in the vertex shader
// GLM provides C++ types that exactly match vector types used
// in shader language
struct Vertex {
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec2 tex_coord;

  // A vertex binding describes at which rate to load data from memory
  // throughout the vertices. It specifies the number of bytes between data
  // entries and whether to move to the next data entry after each vertex or
  // after each instance.
  static vk::VertexInputBindingDescription get_binding_description() {
    // All of our per-vertex data is packed together in one array, so we're
    // only going to have one binding.
    vk::VertexInputBindingDescription binding_description{
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = vk::VertexInputRate::eVertex};

    return binding_description;
  }

  // An attribute description struct describes how to extract a vertex attribute
  // from a chunk of vertex data originating from a binding description. We have
  // three attributes pos, color and texCoord.

  static std::vector<vk::VertexInputAttributeDescription>
  get_attribute_descriptions() {
    std::vector<vk::VertexInputAttributeDescription> attribute_descriptions(3);

    // Tells Vulkan from which binding the per-vertex data comes.
    attribute_descriptions[0].binding = 0;

    // This references the location directive of the input in the vertex shader.
    // e.g layout(location = 0) in vec3 inPosition;
    attribute_descriptions[0].location = 0;

    // a vec3.
    attribute_descriptions[0].format = vk::Format::eR32G32B32Sfloat;

    // offset of x bytes from the beginning of this struct.
    attribute_descriptions[0].offset = offsetof(Vertex, pos);

    // color attribute
    attribute_descriptions[1].binding = 0;
    attribute_descriptions[1].location = 1;
    attribute_descriptions[1].format = vk::Format::eR32G32B32Sfloat;
    attribute_descriptions[1].offset = offsetof(Vertex, color);

    // texture attributes
    attribute_descriptions[2].binding = 0;
    attribute_descriptions[2].location = 2;
    attribute_descriptions[2].format =
        vk::Format::eR32G32Sfloat; // vec2 for texture coordinates
    attribute_descriptions[2].offset = offsetof(Vertex, tex_coord);

#ifndef NDEBUG
    std::cout << std::format("\nOffsets:\n"
                             "pos offset:         {}\n"
                             "color offset:       {}\n"
                             "texCoord offset:    {}\n\n",
                             offsetof(Vertex, pos), offsetof(Vertex, color),
                             offsetof(Vertex, tex_coord));
#endif

    return attribute_descriptions;
  }

  // Operators to enable usage in hash tables:
  bool operator==(const Vertex &other) const {
    return pos == other.pos && color == other.color &&
           tex_coord == other.tex_coord;
  }
};
} // namespace ice

// Hash calculation  for Vertex
// template specialization for std::hash<T>
namespace std {
template <> struct hash<ice::Vertex> {
  size_t operator()(ice::Vertex const &vertex) const {
    // uses GLM gtx/hash.hpp hash functions
    return ((hash<glm::vec3>()(vertex.pos) ^
             (hash<glm::vec3>()(vertex.color) << 1)) >>
            1) ^
           (hash<glm::vec2>()(vertex.tex_coord) << 1);
  }
};
} // namespace std

#endif