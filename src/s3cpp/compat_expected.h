#ifndef S3CPP_COMPAT_EXPECTED_H
#define S3CPP_COMPAT_EXPECTED_H

#include <cassert>
#include <type_traits>
#include <utility>
#include <variant>

namespace s3cpp::compat {

template <typename E>
class unexpected {
public:
  explicit unexpected(E error) : error_(std::move(error)) {}

  const E &error() const & { return error_; }
  E &error() & { return error_; }
  E &&error() && { return std::move(error_); }

private:
  E error_;
};

template <typename E>
unexpected<std::decay_t<E>> make_unexpected(E &&error) {
  return unexpected<std::decay_t<E>>(std::forward<E>(error));
}

template <typename T, typename E>
class expected {
public:
  expected(const T &value) : storage_(value) {}
  expected(T &&value) : storage_(std::move(value)) {}
  template <typename G>
  expected(const unexpected<G> &error) : storage_(E(error.error())) {}
  template <typename G>
  expected(unexpected<G> &&error) : storage_(E(std::move(error).error())) {}

  bool has_value() const noexcept { return std::holds_alternative<T>(storage_); }
  explicit operator bool() const noexcept { return has_value(); }

  T &value() & {
    assert(has_value());
    return std::get<T>(storage_);
  }
  const T &value() const & {
    assert(has_value());
    return std::get<T>(storage_);
  }
  T &&value() && {
    assert(has_value());
    return std::get<T>(std::move(storage_));
  }

  E &error() & {
    assert(!has_value());
    return std::get<E>(storage_);
  }
  const E &error() const & {
    assert(!has_value());
    return std::get<E>(storage_);
  }

  T &operator*() { return value(); }
  const T &operator*() const { return value(); }
  T *operator->() { return &value(); }
  const T *operator->() const { return &value(); }

private:
  std::variant<T, E> storage_;
};

template <typename E>
class expected<void, E> {
public:
  expected() = default;
  template <typename G>
  expected(const unexpected<G> &error)
      : error_(error.error()), has_value_(false) {}
  template <typename G>
  expected(unexpected<G> &&error)
      : error_(std::move(error).error()), has_value_(false) {}

  bool has_value() const noexcept { return has_value_; }
  explicit operator bool() const noexcept { return has_value(); }
  void value() const { assert(has_value()); }

  E &error() & {
    assert(!has_value());
    return error_;
  }
  const E &error() const & {
    assert(!has_value());
    return error_;
  }

private:
  E error_{};
  bool has_value_ = true;
};

} // namespace s3cpp::compat

#endif
