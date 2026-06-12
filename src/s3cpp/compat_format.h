#ifndef S3CPP_COMPAT_FORMAT_H
#define S3CPP_COMPAT_FORMAT_H

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>

namespace s3cpp::compat {
namespace detail {

template <typename T>
std::string format_value(const T &value, std::string_view spec) {
  std::ostringstream output;
  if (spec == ":02X") {
    if constexpr (std::is_integral_v<T> || std::is_enum_v<T>) {
      output << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
             << static_cast<unsigned int>(value);
    } else {
      throw std::runtime_error("Hex format requires an integer");
    }
  } else {
    output << value;
  }
  return output.str();
}

template <typename Duration>
std::string format_value(
    const std::chrono::time_point<std::chrono::system_clock, Duration> &value,
    std::string_view spec) {
  if (spec != ":%Y%m%dT%H%M%SZ") {
    throw std::runtime_error("Unsupported chrono format");
  }

  const std::time_t timestamp = std::chrono::system_clock::to_time_t(
      std::chrono::time_point_cast<std::chrono::system_clock::duration>(value));
  std::tm utc_time{};
#if defined(_WIN32)
  gmtime_s(&utc_time, &timestamp);
#else
  gmtime_r(&timestamp, &utc_time);
#endif
  std::ostringstream output;
  output << std::put_time(&utc_time, "%Y%m%dT%H%M%SZ");
  return output.str();
}

template <std::size_t Index = 0, typename Tuple>
std::string format_argument(
    std::size_t requested_index,
    const Tuple &arguments,
    std::string_view spec) {
  if constexpr (Index < std::tuple_size_v<Tuple>) {
    if (Index == requested_index) {
      return format_value(std::get<Index>(arguments), spec);
    }
    return format_argument<Index + 1>(requested_index, arguments, spec);
  }
  throw std::runtime_error("Not enough format arguments");
}

} // namespace detail

template <typename... Args>
std::string format(std::string_view pattern, Args &&...args) {
  const auto arguments =
      std::forward_as_tuple(std::forward<Args>(args)...);
  std::string output;
  std::size_t argument_index = 0;

  for (std::size_t i = 0; i < pattern.size();) {
    if (pattern[i] != '{') {
      output.push_back(pattern[i++]);
      continue;
    }

    const std::size_t end = pattern.find('}', i + 1);
    if (end == std::string_view::npos) {
      throw std::runtime_error("Invalid format string");
    }
    output += detail::format_argument(
        argument_index++, arguments, pattern.substr(i + 1, end - i - 1));
    i = end + 1;
  }
  return output;
}

} // namespace s3cpp::compat

#endif
