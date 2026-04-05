#pragma once

#include <fstream>
#include <string>

namespace helper {
inline std::string read_file(const std::string &path) {
  std::ifstream file(path, std::ios::ate | std::ios::binary);
  if (!file.is_open())
    return std::string{};
  auto size = file.tellg();
  std::string content;
  content.resize(size);
  file.seekg(0);
  file.read(content.data(), size);
  return content;
}
} // namespace helper