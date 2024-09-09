#include "mesh.hpp"
#include "data_buffers.hpp"
#include <filesystem>

namespace ice {

ObjMesh::ObjMesh(const char *obj_filepath, const char *mtl_filepath,
                 glm::mat4 pre_transform) {
  load(obj_filepath, mtl_filepath, pre_transform);
}

void ObjMesh::load(const char *obj_filepath, const char *mtl_filepath,
                   glm::mat4 pre_transform) {

  this->pre_transform = pre_transform;

  std::ifstream file;
  file.open(mtl_filepath);
  std::string line;
  std::string materialName;
  std::vector<std::string> words;

  while (std::getline(file, line)) {

    words = split(line, " ");

    if (!words[0].compare("newmtl")) {
      materialName = words[1];
    }
    if (!words[0].compare("Kd")) { // get diffuse color
      brush_color = glm::vec3(std::stof(words[1]), std::stof(words[2]),
                              std::stof(words[3]));
      color_lookup.insert({materialName, brush_color});
    }
  }

  file.close();

  file.open(obj_filepath);

  while (std::getline(file, line)) {
    words = split(line, " ");

    if (!words[0].compare("v")) {
      read_vertex_data(words);
    }
    if (!words[0].compare("vt")) {
      read_texcoord_data(words);
    }
    if (!words[0].compare("vn")) {
      read_normal_data(words);
    }
    if (!words[0].compare("usemtl")) {
      if (color_lookup.contains(words[1])) {
        brush_color = color_lookup[words[1]];
      } else {
        brush_color = glm::vec3(1.0f); // just color white
      }
    }
    if (!words[0].compare("f")) {
      read_face_data(words);
    }
  }

  file.close();
}

void ObjMesh::read_vertex_data(const std::vector<std::string> &words) {
  // extra component for homogenous coords
  glm::vec4 new_vertex = glm::vec4(std::stof(words[1]), std::stof(words[2]),
                                   std::stof(words[3]), 1.0f);
  glm::vec3 transformed_vertex = glm::vec3(pre_transform * new_vertex);
  v.push_back(transformed_vertex);
}

void ObjMesh::read_texcoord_data(const std::vector<std::string> &words) {
  glm::vec2 new_texcoord = glm::vec2(std::stof(words[1]), std::stof(words[2]));
  vt.push_back(new_texcoord);
}

void ObjMesh::read_normal_data(const std::vector<std::string> &words) {
  // component of 0 for normal data
  glm::vec4 new_normal = glm::vec4(std::stof(words[1]), std::stof(words[2]),
                                   std::stof(words[3]), 0.0f);
  glm::vec3 transformed_normal = glm::vec3(pre_transform * new_normal);
  vn.push_back(transformed_normal);
}

void ObjMesh::read_face_data(const std::vector<std::string> &words) {

  size_t triangle_count = words.size() - 3; // size - 2 , extra 1 for f

  // triangles formed like this  1, 2, 3 then 1, 3, 4 then 1, 4, 5 ...
  for (int i = 0; i < triangle_count; ++i) {
    read_corner(words[1]);
    read_corner(words[2 + i]);
    read_corner(words[3 + i]);
  }
}

void ObjMesh::read_corner(const std::string &vertex_description) {

  if (history.contains(vertex_description)) {
    indices.push_back(history[vertex_description]);
    return;
  }

  uint32_t index = static_cast<uint32_t>(history.size());
  history.insert({vertex_description, index});
  indices.push_back(index);

  std::vector<std::string> v_vt_vn = split(vertex_description, "/");

  // prepare attributes
  // position
  glm::vec3 pos = v[std::stol(v_vt_vn[0]) - 1]; // obj uses 1 based indexing

  // texcoord
  glm::vec2 texcoord = glm::vec2(0.0f, 0.0f);
  if (v_vt_vn.size() == 3 && v_vt_vn[1].size() > 0) {
    texcoord = vt[std::stol(v_vt_vn[1]) - 1];
  }

  // normals
  glm::vec3 normal = vn[std::stol(v_vt_vn[2]) - 1];

  // Append Vertex
  vertices.push_back(Vertex{.pos = pos,
                            .color = brush_color,
                            .tex_coord = texcoord,
                            .normal = normal});
}

///////////////////////////////////////////////////////////////
////////////////////////// GLTF MESH //////////////////////////
///////////////////////////////////////////////////////////////

// Utilities
namespace {
// utility function for resolving paths
std::string make_path_relative_to_gltf(const std::string &gltf_filepath,
                                       const std::string &uri) {
  namespace fs = std::filesystem;
  fs::path gltf_dir = fs::path(gltf_filepath).parent_path();

  // Combine the GLTF directory with the URI
  fs::path full_path = gltf_dir / uri;
  full_path = fs::absolute(full_path).lexically_normal();

  return full_path.string();
}
} // namespace

GltfMesh::~GltfMesh() {
  for (auto &mesh_buffer : mesh_buffers) {
    // Destroy  mesh vertex and index buffers
    device.destroyBuffer(mesh_buffer.vertex_buffer.buffer);
    device.freeMemory(mesh_buffer.vertex_buffer.buffer_memory);

    device.destroyBuffer(mesh_buffer.index_buffer.buffer);
    device.freeMemory(mesh_buffer.index_buffer.buffer_memory);
  }

  device.destroyDescriptorPool(descriptor_pool);

  for (auto texture : textures) {
    delete texture;
  }
}

GltfMesh::GltfMesh(vk::PhysicalDevice physical_device, vk::Device device,
                   vk::CommandBuffer command_buffer, vk::Queue queue,
                   vk::DescriptorSetLayout descriptor_set_layout,
                   vk::DescriptorPool descriptor_pool,
                   const char *gltf_filepath, glm::mat4 pre_transform)
    : physical_device(physical_device), device(device),
      command_buffer(command_buffer), queue(queue),
      descriptor_set_layout(descriptor_set_layout),
      descriptor_pool(descriptor_pool), pre_transform(pre_transform),
      gltf_filepath(gltf_filepath) {
  load(gltf_filepath);
}

void GltfMesh::load(const char *gltf_filepath) {
  if (ice::load_gltf_model(model, gltf_filepath) == false) {
#ifndef NDEBUG
    std::cerr << "Loading of " << gltf_filepath << " failed";
#endif
    throw std::runtime_error(std::string("Error loading ") + gltf_filepath);
  }
#ifndef NDEBUG
  debug_model();
#endif

  bind_models();
}

glm::mat4 GltfMesh::get_local_transform(const tinygltf::Node &node) {
  if (!node.matrix.empty()) {
    return glm::make_mat4(node.matrix.data());
  }

  glm::mat4 transform(1.0f);

  if (node.translation.size() == 3) {
    transform = glm::translate(transform, glm::vec3(node.translation[0],
                                                    node.translation[1],
                                                    node.translation[2]));
  }

  if (node.rotation.size() == 4) {
    glm::quat q(static_cast<float>(node.rotation[3]),
                static_cast<float>(node.rotation[0]),
                static_cast<float>(node.rotation[1]),
                static_cast<float>(node.rotation[2]));
    transform = transform * glm::mat4_cast(q);
  }

  if (node.scale.size() == 3) {
    transform = glm::scale(
        transform, glm::vec3(node.scale[0], node.scale[1], node.scale[2]));
  }

  return transform;
}

void GltfMesh::bind_models() {
  const tinygltf::Scene &scene = model.scenes[model.defaultScene];

  for (std::size_t i = 0; i < scene.nodes.size(); ++i) {
    assert((scene.nodes[i] >= 0) && (scene.nodes[i] < model.nodes.size()));
    bind_model_nodes(model.nodes[scene.nodes[i]], pre_transform);
  }
}

// recursively binds nodes
void GltfMesh::bind_model_nodes(tinygltf::Node &node,
                                glm::mat4 parent_transform) {
  glm::mat4 local_transform = get_local_transform(node);
  glm::mat4 global_transform = parent_transform * local_transform;

  if ((node.mesh >= 0) && (node.mesh < model.meshes.size())) {
    bind_mesh(model.meshes[node.mesh], global_transform);
  }

  for (size_t i = 0; i < node.children.size(); i++) {
    assert((node.children[i] >= 0) && (node.children[i] < model.nodes.size()));
    bind_model_nodes(model.nodes[node.children[i]], global_transform);
  }
}

void GltfMesh::bind_mesh(tinygltf::Mesh &mesh,
                         const glm::mat4 &global_transform) {
  for (const tinygltf::Primitive &primitive : mesh.primitives) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    // Get accessors (with error checking)
    auto get_accessor =
        [&](const char *attr_name) -> const tinygltf::Accessor * {
      auto it = primitive.attributes.find(attr_name);
#ifndef NDEBUG
      if (it == primitive.attributes.end()) {
        std::cerr
            << std::format(
                   "Warning: Mesh primitive doesn't have {} data. Skipping.",
                   attr_name)
            << std::endl;
      }

#endif
      return (it != primitive.attributes.end()) ? &model.accessors[it->second]
                                                : nullptr;
    };

    const tinygltf::Accessor *pos_accessor = get_accessor("POSITION");
    const tinygltf::Accessor *normal_accessor = get_accessor("NORMAL");
    const tinygltf::Accessor *texcoord_accessor = get_accessor("TEXCOORD_0");
    const tinygltf::Accessor *color_accessor = get_accessor("COLOR_0");

    // Ensure we at least have position data
    if (!pos_accessor) {
      std::cerr << "Position is Required " << std::endl;
      continue;
    }

    // Reserve space for vertices
    const std::size_t vertex_count = pos_accessor->count;
    vertices.reserve(vertex_count);

    // Helper function to get buffer data
    auto get_buffer_data =
        [this](const tinygltf::Accessor *accessor) -> const uint8_t * {
      if (!accessor)
        return nullptr;
      const tinygltf::BufferView &bufferView =
          model.bufferViews[accessor->bufferView];
      const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];
      return &buffer.data[bufferView.byteOffset + accessor->byteOffset];
    };

    const float *pos_data =
        reinterpret_cast<const float *>(get_buffer_data(pos_accessor));
    const float *normal_data =
        reinterpret_cast<const float *>(get_buffer_data(normal_accessor));
    const float *texcoord_data =
        reinterpret_cast<const float *>(get_buffer_data(texcoord_accessor));
    const float *color_data =
        reinterpret_cast<const float *>(get_buffer_data(color_accessor));

    for (size_t i = 0; i < vertex_count; ++i) {
      Vertex vertex;

      vertex.pos =
          glm::vec3(pos_data[i * 3], pos_data[i * 3 + 1], pos_data[i * 3 + 2]);

      // Normal (optional)
      if (normal_data) {
        vertex.normal = glm::vec3(normal_data[i * 3], normal_data[i * 3 + 1],
                                  normal_data[i * 3 + 2]);
      } else {
        vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f); // Default normal
      }

      // Apply global transform to vertices
      glm::vec4 transformed = global_transform * glm::vec4(vertex.pos, 1.0f);
      vertex.pos = glm::vec3(transformed) / transformed.w;
      vertex.normal =
          glm::mat3(glm::transpose(glm::inverse(global_transform))) *
          vertex.normal;

      // Texture coordinates (optional)
      if (texcoord_data) {
        vertex.tex_coord =
            glm::vec2(texcoord_data[i * 2], texcoord_data[i * 2 + 1]);
      } else {
        vertex.tex_coord = glm::vec2(0.0f, 0.0f); // Default UV
      }

      // Color (optional)
      if (color_data) {
        if (color_accessor->componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
          vertex.color = glm::vec3(color_data[i * 3], color_data[i * 3 + 1],
                                   color_data[i * 3 + 2]);
        } else if (color_accessor->componentType ==
                   TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
          const uint8_t *color_data_byte =
              reinterpret_cast<const uint8_t *>(color_data);
          vertex.color = glm::vec3(color_data_byte[i * 3] / 255.0f,
                                   color_data_byte[i * 3 + 1] / 255.0f,
                                   color_data_byte[i * 3 + 2] / 255.0f);
        }
      } else {
        vertex.color = glm::vec3(1.0f, 1.0f, 1.0f); // Default color (white)
      }

      vertices.push_back(vertex);
    }

    // TODO: BENCHMARK THIS
    // // Apply global transform to vertices as a batch -
    // for (auto &vertex : vertices) {
    //   glm::vec4 transformed = global_transform * glm::vec4(vertex.pos, 1.0f);
    //   vertex.pos = glm::vec3(transformed) / transformed.w;
    //   vertex.normal =
    //       glm::mat3(glm::transpose(glm::inverse(global_transform))) *
    //       vertex.normal;
    // }

    // Read index data (if available)
    if (primitive.indices >= 0) {
      const tinygltf::Accessor &index_accessor =
          model.accessors[primitive.indices];
      const tinygltf::BufferView &index_bufferView =
          model.bufferViews[index_accessor.bufferView];
      const tinygltf::Buffer &index_buffer =
          model.buffers[index_bufferView.buffer];
      const uint8_t *index_data =
          &index_buffer
               .data[index_bufferView.byteOffset + index_accessor.byteOffset];

      indices.reserve(index_accessor.count);

      if (index_accessor.componentType ==
          TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
        const uint16_t *index_data_short =
            reinterpret_cast<const uint16_t *>(index_data);
        for (size_t i = 0; i < index_accessor.count; ++i) {
          indices.push_back(static_cast<uint32_t>(index_data_short[i]));
        }
      } else if (index_accessor.componentType ==
                 TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
        const uint32_t *index_data_int =
            reinterpret_cast<const uint32_t *>(index_data);
        for (size_t i = 0; i < index_accessor.count; ++i) {
          indices.push_back(index_data_int[i]);
        }
      } else {
#ifndef NDEBUG
        std::cerr
            << "Warning: Unsupported index component type. Skipping indices."
            << std::endl;
#endif
      }
    } else {
      // If no indices are provided, create a simple index buffer
      indices.reserve(vertices.size());
      for (size_t i = 0; i < vertices.size(); ++i) {
        indices.push_back(static_cast<uint32_t>(i));
      }
    }

    // Buffers creation
    ice::BufferBundle vertex_buffer_bundle = create_device_local_buffer(
        physical_device, device, command_buffer, queue,
        vk::BufferUsageFlagBits::eVertexBuffer, vertices);
    ice::BufferBundle index_buffer_bundle = create_device_local_buffer(
        physical_device, device, command_buffer, queue,
        vk::BufferUsageFlagBits::eIndexBuffer, indices);

    // Store the buffer pair
    mesh_buffers.push_back({vertex_buffer_bundle, index_buffer_bundle});
    index_counts.push_back(static_cast<uint32_t>(indices.size()));

    // Handle material and texture
    if (primitive.material >= 0) {
      const tinygltf::Material &material = model.materials[primitive.material];

      // Check for base color texture
      if (material.pbrMetallicRoughness.baseColorTexture.index >= 0) {
        int texture_index =
            material.pbrMetallicRoughness.baseColorTexture.index;
        const tinygltf::Texture &gltf_texture = model.textures[texture_index];
        const tinygltf::Image &gltf_image = model.images[gltf_texture.source];

        // Create texture
        ice_image::TextureCreationInput texture_input{
            .physical_device = physical_device,
            .logical_device = device,
            .command_buffer = command_buffer,
            .queue = queue,
            .layout = descriptor_set_layout,
            .descriptor_pool = descriptor_pool,
            .filenames = {}};

        ice_image::Texture *texture;
        if (!gltf_image.uri.empty()) {
          // External image file
          std::string relative_path =
              make_path_relative_to_gltf(gltf_filepath, gltf_image.uri);
          texture_input.filenames.push_back(relative_path.c_str());
#ifndef NDEBUG
          std::cout << "\nFILENAME\n " << texture_input.filenames[0] << "\n\n";
#endif
          texture = new ice_image::Texture(texture_input);
        } else if (gltf_image.image.size() > 0) {
          // read image data from embedded buffer
          texture = new ice_image::Texture(
              texture_input, std::make_shared<tinygltf::Image>(gltf_image));
        }
        textures.push_back(texture);
      }
    }
  }
}

#ifndef NDEBUG
void GltfMesh::debug_model() {
  for (auto &mesh : model.meshes) {
    std::cout << "mesh : " << mesh.name << std::endl;
    for (auto &primitive : mesh.primitives) {
      const tinygltf::Accessor &index_accessor =
          model.accessors[primitive.indices];

      std::cout << "index accessor: count " << index_accessor.count << ", type "
                << index_accessor.componentType << std::endl;

      tinygltf::Material &mat = model.materials[primitive.material];
      for (auto &mats : mat.values) {
        std::cout << "mat : " << mats.first.c_str() << std::endl;
      }

      for (auto &image : model.images) {
        std::cout << "image name : " << image.uri << std::endl;
        std::cout << "  size : " << image.image.size() << std::endl;
        std::cout << "  w/h : " << image.width << "/" << image.height
                  << std::endl;
      }

      std::cout << "indices : " << primitive.indices << std::endl;
      std::cout << "mode     : "
                << "(" << primitive.mode << ")" << std::endl;

      for (auto &attrib : primitive.attributes) {
        std::cout << "attribute : " << attrib.first.c_str() << std::endl;
      }
    }
  }
}
#endif

void GltfMesh::update_transforms(glm::mat4 new_transform) {
  pre_transform = new_transform;

  mesh_buffers.clear();
  index_counts.clear();

  // Rebind all models with potentially updated transforms
  bind_models();
}

} // namespace ice
