#pragma once

#include <cassert>
#include <optional>
#include <system_error>


#define TRY(error_or) do { if (auto e = (error_or).err()) { return e; } } while (false)
#define FAIL(code) return make_error_code(code)

namespace shar {

using ErrorCode = std::error_code;

template <typename T>
class ErrorOr {
public:
  ErrorOr(T&& value)
    : m_value(std::forward<T>(value))
    {}

  ErrorOr(const T& value)
    : m_value(value)
    {}

  ErrorOr(ErrorCode ec)
    : m_code(std::move(ec))
    {}

  ErrorOr(const ErrorOr&) = default;
  ErrorOr(ErrorOr&&) = default;
  ErrorOr& operator=(const ErrorOr&) = default;
  ErrorOr& operator=(ErrorOr&&) = default;
  ~ErrorOr() = default;

  T* operator->() {
    assert(m_value.has_value());
    return &*m_value;
  }

  const T* operator->() const {
    assert(m_value.has_value());
    return &*m_value;
  }

  T& operator*() {
    assert(m_value.has_value());
    return *m_value;
  }

  const T& operator*() const {
    assert(m_value.has_value());
    return *m_value;
  }

  ErrorCode err() const {
    return m_code;
  }

private:
  std::optional<T> m_value;
  ErrorCode m_code;
};

}