#pragma once

#include "int.hpp"


namespace shar {

// non-owning mutable reference to bytes
class BytesRef {
public:
  BytesRef() noexcept = default;
  BytesRef(u8* data, usize size) noexcept
    : m_data(data)
    , m_size(size)
  {}

  BytesRef(const BytesRef&) noexcept = default;
  BytesRef& operator=(const BytesRef&) noexcept = default;
  ~BytesRef() = default;

  operator bool() const {
    return m_data != nullptr;
  }

  const u8* data() const noexcept {
    return m_data;
  }

  u8* data() noexcept {
    return m_data;
  }

  usize size() const noexcept {
    return m_size;
  }

protected:
  u8* m_data{nullptr};
  usize m_size{0};
};

}