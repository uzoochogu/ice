#ifndef LOADERS_HPP
#define LOADERS_HPP

#include <tiny_gltf.h>

#include "config.hpp"

namespace ice {
// Loading Shaders

// @brief Reads all bytes from specified file and return a byte array.
inline std::vector<char> read_file(const std::string &filename) {
  std::ifstream file(
      filename,
      std::ios::ate | std::ios::binary);  // start reading at the end of file.

  if (!file.is_open()) {
#ifndef NDEBUG
    std::cerr << "Failed to open file" << std::endl;
#endif
    throw std::runtime_error("failed to open file!");
  }

  // determine size of file  using read position
  const std::size_t file_size = static_cast<std::size_t>(file.tellg());
  std::vector<char> buffer(file_size);

  // seek to beginning
  file.seekg(0);
  file.read(buffer.data(), static_cast<std::streamsize>(file_size));

  // close file and return bytes
  file.close();
  return buffer;
}

inline vk::ShaderModule create_shader_module(std::string filename,
                                             vk::Device device) {
  std::vector<char> source_code = read_file(filename);

  constexpr std::uint32_t SPIRV_MAGIC = 0x07230203;
  constexpr std::uint32_t SPIRV_MAGIC_REV = 0x03022307;
  std::uint32_t magic{0};

  if (source_code.size() < sizeof(std::uint32_t)) {
    throw std::runtime_error("Invalid shader code");
  }

  memcpy(&magic, source_code.data(), sizeof(magic));
  if (magic != SPIRV_MAGIC &&
      magic != SPIRV_MAGIC_REV) {  // account for endianness??
#ifndef NDEBUG
    std::cout << std::format(
        "Magic number for spir-v file {} is {:#08x} -- should be {:#08x}\n",
        filename, magic, SPIRV_MAGIC);
#endif

    throw std::runtime_error("Incorrect SPIRV_MAGIC");
  }

#ifndef NDEBUG
  std::cout << std::format("Shader has correct SPIR-V magic {:#08x}\n",
                           SPIRV_MAGIC);
#endif

  const vk::ShaderModuleCreateInfo module_info = {
      .flags = vk::ShaderModuleCreateFlags(),
      .codeSize = source_code.size(),
      .pCode = reinterpret_cast<const uint32_t *>(source_code.data())};
  try {
    return device.createShaderModule(module_info);
  } catch (const vk::SystemError &err) {
    throw std::runtime_error(
        std::format("Failed to create shader module for {}", filename));
  }
}

inline bool load_gltf_model(tinygltf::Model &model, const char *filename) {
  tinygltf::TinyGLTF loader;
  std::string err;
  std::string warn;

  bool res = loader.LoadASCIIFromFile(&model, &err, &warn, filename);

  if (!res) {
    res = loader.LoadBinaryFromFile(&model, &err, &warn,
                                    filename);  // for binary glTF(.glb)
  }

#ifndef NDEBUG
  if (!warn.empty()) {
    std::cout << "WARN: " << warn << std::endl;
  }

  if (!err.empty()) {
    std::cout << "ERR: " << err << std::endl;
  }

  if (!res) {
    std::cout << "Failed to load glTF: " << filename << std::endl;
  } else {
    std::cout << "Loaded glTF: " << filename << std::endl;
  }
#endif

  return res;
}

}  // namespace ice

#endif  //  LOADERS_HPP
