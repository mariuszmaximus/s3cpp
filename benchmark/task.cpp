#include "task.h"
#include <format>
#include <string>

extern "C" {
const char* get_object(const char* key) noexcept {
  static std::string result;
  result = std::format("Hello from C++!\nKey: {}", key);
  return result.c_str();
}
}
