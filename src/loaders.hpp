#ifndef LOADERS_HPP
#define LOADERS_HPP

#include "config.hpp"

namespace ice {
// Loading Shaders

// @brief Reads all bytes from specified file and return a byte array.
inline std::vector<char> read_file(const std::string &filename) {
  std::ifstream file(filename,
                     std::ios::ate |
                         std::ios::binary); // start reading at the end of file.

  if (!file.is_open()) {
    std::cerr << "Failed to open file" << std::endl;
    throw std::runtime_error("failed to open file!");
  }

  // determine size of file  using read position
  std::size_t file_size = static_cast<std::size_t>(file.tellg());
  std::vector<char> buffer(file_size); // allocate buffer to file size

  // seek to beginning
  file.seekg(0);
  file.read(buffer.data(), file_size);

  // close file and return bytes
  file.close();
  return buffer;
}

inline vk::ShaderModule create_shader_module(std::string filename,
                                             vk::Device device) {
  std::vector<char> source_code = read_file(filename);
  vk::ShaderModuleCreateInfo module_info = {
      .flags = vk::ShaderModuleCreateFlags(),
      .codeSize = source_code.size(),
      .pCode = reinterpret_cast<const uint32_t *>(source_code.data())};
  try {
    return device.createShaderModule(module_info);
  } catch (vk::SystemError err) {
    throw std::runtime_error(
        std::format("Failed to create shader module for {}", filename));
  }
}

/* void loadModel() {
  // this container holds all of the positions(attrib.vertices),
  // normals(.normals) and texture coords(.textcoords)
  tinyobj::attrib_t attrib;
  // contains all the separate objects and their faces. Each face consists of
  // an array of vertices(each containing position, normal and texture coord
  // attributes). LoadObj triangulates faces by default.
  std::vector<tinyobj::shape_t> shapes;
  // Also define material and texture per face but will be ignored here.
  std::vector<tinyobj::material_t> materials;
  std::string warn, err; // to store warning during load

  // loads model into libs data structures
  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                        MODEL_PATH.c_str())) {
    throw std::runtime_error(warn + err);
  }

  // will be used to prevent vertex duplication
  inline std::unordered_map<Vertex, std::uint32_t> uniqueVertices{};

  for (const auto &shape : shapes) {
    // Triangulation already in effect, 3 vertices per face.
    // index contains tinyobj::index_t::vertex_index, ::normal::index and
    // texcoord_index. We use these indices to look up actual vertex
    // attributes in the attrib arrays
    for (const auto &index : shape.mesh.indices) {
      Vertex vertex{};

      if (index.vertex_index >= 0) {
        // multiply by 3 and offset because it is an array of floats and not
        // like glm::vec3
        vertex.pos = {attrib.vertices[3 * index.vertex_index + 0],
                      attrib.vertices[3 * index.vertex_index + 1],
                      attrib.vertices[3 * index.vertex_index + 2]};

        vertex.color = {
            attrib.colors[3 * index.vertex_index + 0],
            attrib.colors[3 * index.vertex_index + 1],
            attrib.colors[3 * index.vertex_index + 2],
        };

        // vertex.color = {1.0f, 1.0f, 1.0f}; // vertex default colour
      }

      // // Useful if we had normals
      //  if (index.normal_index >= 0) {
      //   vertex.normal = {
      //       attrib.normals[3 * index.normal_index + 0],
      //       attrib.normals[3 * index.normal_index + 1],
      //       attrib.normals[3 * index.normal_index + 2],
      //   };
      // }

      if (index.texcoord_index >= 0) {
        //  similar multiplication. flip the vertical component of texture
        //  coordinates since we uploaded image to Vulkan in a top(0) to
        //  bottom(1) orientation rather than bottom(0) to top(1) of OBJ
        //  format.
        vertex.texCoord = {attrib.texcoords[2 * index.texcoord_index + 0],
                           1.0f -
                               attrib.texcoords[2 * index.texcoord_index +
1]};
      }

      // check if we have seen vertex with exact same position, color and
      // texture coords if not, we add it to vertices
      if (uniqueVertices.count(vertex) == 0) {
        // store an index for that vertex (key)
        uniqueVertices[vertex] = static_cast<std::uint32_t>(vertices.size());
        vertices.push_back(vertex);
      }

      // lookup and use vertex index
      indices.push_back(uniqueVertices[vertex]);
    }
  }
} */
} // namespace ice

#endif