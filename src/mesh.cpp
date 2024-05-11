#include "mesh.hpp"

namespace ice {

ObjMesh::ObjMesh(const char *obj_filepath, const char *mtl_filepath,
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
} // namespace ice