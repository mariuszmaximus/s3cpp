#include <format>
#include <string>

extern "C" {

const char *init(const char *access, const char *secret,
                 const char *endpoint) noexcept {
  static std::string result;
  result = std::format("c++: access: {}", access);
  return result.c_str();
}

const char *get_object(const char *key) noexcept {
  static std::string result;
  result = std::format("c++: Key: {}", key);
  return result.c_str();
}
}
