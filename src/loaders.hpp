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
} // namespace ice

#endif
