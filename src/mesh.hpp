#ifndef MESH_HPP
#define MESH_HPP

#include "data_buffers.hpp"
#include "game_objects.hpp"
#include "images/ice_texture.hpp"
#include "loaders.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/hash.hpp> // for hashing glm::vec data structures

namespace ice {

// Vertex data
// To be used in the vertex shader
struct Vertex {
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec2 tex_coord;
  glm::vec3 normal;

  static vk::VertexInputBindingDescription get_binding_description() {
    vk::VertexInputBindingDescription binding_description{
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = vk::VertexInputRate::eVertex};

    return binding_description;
  }

  static std::vector<vk::VertexInputAttributeDescription>
  get_attribute_descriptions() {
    std::vector<vk::VertexInputAttributeDescription> attribute_descriptions(4);

    attribute_descriptions[0].binding = 0;

    attribute_descriptions[0].location = 0;

    attribute_descriptions[0].format = vk::Format::eR32G32B32Sfloat; // vec3.

    attribute_descriptions[0].offset = offsetof(Vertex, pos);

    // color attribute
    attribute_descriptions[1].binding = 0;
    attribute_descriptions[1].location = 1;
    attribute_descriptions[1].format = vk::Format::eR32G32B32Sfloat;
    attribute_descriptions[1].offset = offsetof(Vertex, color);

    // texture attributes
    attribute_descriptions[2].binding = 0;
    attribute_descriptions[2].location = 2;
    attribute_descriptions[2].format = vk::Format::eR32G32Sfloat; // vec2
    attribute_descriptions[2].offset = offsetof(Vertex, tex_coord);

    // normal attribute
    attribute_descriptions[3].binding = 0;
    attribute_descriptions[3].location = 3;
    attribute_descriptions[3].format = vk::Format::eR32G32B32Sfloat;
    attribute_descriptions[3].offset = offsetof(Vertex, normal);

#ifndef NDEBUG
    std::cout << std::format("\nOffsets:\n"
                             "pos offset:         {}\n"
                             "color offset:       {}\n"
                             "texCoord offset:    {}\n"
                             "normal offset:      {}\n\n",
                             offsetof(Vertex, pos), offsetof(Vertex, color),
                             offsetof(Vertex, tex_coord),
                             offsetof(Vertex, normal));
#endif

    return attribute_descriptions;
  }

  // Operators to enable usage in hash tables:
  bool operator==(const Vertex &other) const {
    return pos == other.pos && color == other.color &&
           tex_coord == other.tex_coord && normal == other.normal;
  }
};

// loads mesh data from Obj and corresponding mtl files
class ObjMesh {
public:
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  std::vector<glm::vec3> v, vn;
  std::vector<glm::vec2> vt;
  std::unordered_map<std::string, uint32_t> history;
  std::unordered_map<std::string, glm::vec3> color_lookup;
  glm::vec3 brush_color;
  glm::mat4 pre_transform;

  // Methods

  // Defers loading to when load() is called
  ObjMesh() = default;

  // construct and load
  ObjMesh(const char *obj_filepath, const char *mtl_filepath,
          glm::mat4 pre_transform);

  void load(const char *obj_filepath, const char *mtl_filepath,
            glm::mat4 pre_transform);

  void read_vertex_data(const std::vector<std::string> &words);

  void read_texcoord_data(const std::vector<std::string> &words);

  void read_normal_data(const std::vector<std::string> &words);

  void read_face_data(const std::vector<std::string> &words);

  void read_corner(const std::string &vertex_description);
};

struct MeshBuffer {
  ice::BufferBundle vertex_buffer;
  ice::BufferBundle index_buffer;
};

// loads mesh data from GLTF, it can represent a whole scene
class GltfMesh {
public:
  GltfMesh() = default; // defers loading
  ~GltfMesh();

  GltfMesh(vk::PhysicalDevice physical_device, vk::Device device,
           vk::CommandBuffer command_buffer, vk::Queue queue,
           vk::DescriptorSetLayout descriptor_set_layout,
           vk::DescriptorPool descriptor_pool, const char *obj_filepath,
           glm::mat4 pre_transform);

  // new transform to update the mesh with
  void update_transforms(glm::mat4 new_transform);

  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;

  std::vector<MeshBuffer> mesh_buffers;
  std::vector<uint32_t> index_counts;

  std::vector<ice_image::Texture *> textures;

private:
  void load(const char *gltf_filepath);
  void bind_models();
  void bind_model_nodes(tinygltf::Node &node,
                        glm::mat4 parent_transform = glm::mat4(1.0f));
  void bind_mesh(tinygltf::Mesh &mesh, const glm::mat4 &global_transform);
  glm::mat4 get_local_transform(const tinygltf::Node &node);

#ifndef NDEBUG
  void debug_model();
#endif

  tinygltf::Model model;

  glm::mat4 pre_transform;
  std::string gltf_filepath;

  vk::PhysicalDevice physical_device;
  vk::Device device;
  vk::CommandBuffer command_buffer;
  vk::Queue queue;
  vk::DescriptorSetLayout descriptor_set_layout;
  vk::DescriptorPool descriptor_pool;
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
           (hash<glm::vec2>()(vertex.tex_coord) ^
            (hash<glm::vec3>()(vertex.normal) << 1));
  }
};
} // namespace std

#endif
