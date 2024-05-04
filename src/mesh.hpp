#ifndef MESH_HPP
#define MESH_HPP

#include "game_objects.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp> // for hashing glm::vec data structures

namespace ice {

// /**
//         \returns the input binding description for a (vec2 pos, vec3 color)
//    vertex format.
// */
// inline vk::VertexInputBindingDescription getPosColorBindingDescription() {

//   /* Provided by VK_VERSION_1_0
//   typedef struct VkVertexInputBindingDescription {
//           uint32_t             binding;
//           uint32_t             stride;
//           VkVertexInputRate    inputRate;
//   } VkVertexInputBindingDescription;
//   */

//   vk::VertexInputBindingDescription bindingDescription;
//   bindingDescription.binding = 0;
//   bindingDescription.stride = 5 * sizeof(float);
//   bindingDescription.inputRate = vk::VertexInputRate::eVertex;

//   return bindingDescription;
// }

// /**
//         \returns the input attribute descriptions for a (vec2 pos, vec3
//         color)
//    vertex format.
// */
// inline std::array<vk::VertexInputAttributeDescription, 2>
// getPosColorAttributeDescriptions() {

//   /* Provided by VK_VERSION_1_0
//   typedef struct VkVertexInputAttributeDescription {
//           uint32_t    location;
//           uint32_t    binding;
//           VkFormat    format;
//           uint32_t    offset;
//   } VkVertexInputAttributeDescription;
//   */

//   std::array<vk::VertexInputAttributeDescription, 2> attributes;

//   // Pos
//   attributes[0].binding = 0;
//   attributes[0].location = 0;
//   attributes[0].format = vk::Format::eR32G32Sfloat;
//   attributes[0].offset = 0;

//   // Color
//   attributes[1].binding = 0;
//   attributes[1].location = 1;
//   attributes[1].format = vk::Format::eR32G32B32Sfloat;
//   attributes[1].offset = 2 * sizeof(float);

//   return attributes;
// }

// Vertex data
// To be used in the vertex shader
// GLM provides C++ types that exactly match vector types used
// in shader language
struct Vertex {
  glm::vec3 pos;
  glm::vec3 color;
  // glm::vec2 texCoord;

  // A vertex binding describes at which rate to load data from memory
  // throughout the vertices. It specifies the number of bytes between data
  // entries and whether to move to the next data entry after each vertex or
  // after each instance.
  static vk::VertexInputBindingDescription getBindingDescription() {
    // All of our per-vertex data is packed together in one array, so we're
    // only going to have one binding.
    vk::VertexInputBindingDescription bindingDescription{
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = vk::VertexInputRate::eVertex};

    return bindingDescription;
  }

  // An attribute description struct describes how to extract a vertex attribute
  // from a chunk of vertex data originating from a binding description. We have
  // three attributes pos, color and texCoord.

  // Todo change to 3
  static std::array<vk::VertexInputAttributeDescription, 2>
  getAttributeDescriptions() {
    // Todo change to 3
    std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions{};

    // Tells Vulkan from which binding the per-vertex data comes.
    attributeDescriptions[0].binding = 0;

    // This references the location directive of the input in the vertex shader.
    // e.g layout(location = 0) in vec2 inPosition;
    // 0 is the position and has two 32-bit float components.
    attributeDescriptions[0].location = 0;

    // Describes the type of data for the attribute.
    // Albeit confusingly, they are specified using the same enumeration as
    // color format. e.g. float: VK_FORMAT_R32_SFLOAT. use the format where the
    // amount of color channels matches the number of components in the shader
    // data type. Below is for a vec3.
    attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;

    // format implicitly defines the byte size of attribute data and the offset
    // parameter specifies the number of bytes since the start of the
    // per-vertex data to read from. The offset macro calculates the offest
    // based on the fact that the binding is loading one Vertex at a time and
    // the position attribute (pos) is at an offset of 0 bytes from the
    // beginning of this struct.
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    // color attribute is described similarly
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    // texture attributes
    /*     attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format =
            vk::Format::eR32G32Sfloat; // vec2 for texture coordinates
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord); */

#ifndef NDEBUG
    std::cout << std::format("\nOffsets:\n"
                             "pos offset:         {}\n"
                             "color offset:       {}\n"
                             "texCoord offset:    {}\n\n",
                             offsetof(Vertex, pos), offsetof(Vertex, color),
                             2 * offsetof(Vertex, color) -
                                 offsetof(Vertex, pos));
#endif

    return attributeDescriptions;
  }

  // Operators to enable usage in hash tables:
  bool operator==(const Vertex &other) const {
    return pos == other.pos && color == other.color /*  &&
            texCoord == other.texCoord */
        ;
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
            1) /* ^
           (hash<glm::vec2>()(vertex.texCoord) << 1) */
        ;
  }
};
} // namespace std

#endif