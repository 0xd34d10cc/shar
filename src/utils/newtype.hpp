#pragma once

#include <utility>


namespace shar {

template <typename T>
class NewType {
public:
  NewType() = delete;
  explicit NewType(T value)
      : m_value(std::move(value))
  {}
  NewType(const NewType&) = default;
  NewType(NewType&&) noexcept = default;
  NewType& operator=(const NewType&) = default;
  NewType& operator=(NewType&&) noexcept = default;
  ~NewType() = default;

  T& get() & {
    return m_value;
  }

  T&& get() && {
    return std::move(m_value);
  }

  const T& get() const& {
    return m_value;
  }

  void set(T value) {
    m_value = std::move(value);
  }

private:
  T m_value;
};

}