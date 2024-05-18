#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <array>
#include <format>
#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

// Todo, create wrappers ifdef wrappers to give choice in using structs or not
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>

// Pipeline types used in the engine
enum class PipelineType { SKY, STANDARD };


inline std::vector<std::string> split(std::string line, std::string delimiter) {

  std::vector<std::string> split_line;

  size_t pos = 0;
  std::string token;
  while ((pos = line.find(delimiter)) != std::string::npos) {
    token = line.substr(0, pos);
    split_line.push_back(token);
    line.erase(0, pos + delimiter.length());
  }
  split_line.push_back(line);

  return split_line;
}

#endif