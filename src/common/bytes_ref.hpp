#pragma once

#include "int.hpp"

#include <cassert>
#include <cstring>

namespace shar {

// non-owning const reference to bytes
class BytesRef {
public:
  BytesRef() noexcept = default;
  BytesRef(const u8* data, usize size) noexcept : m_data(data), m_size(size) {}
  BytesRef(const u8* begin, const u8* end) noexcept
      : m_data(begin)
      , m_size(static_cast<usize>(end - begin)) {}

  BytesRef(const char* s, usize size)
      : BytesRef(reinterpret_cast<const u8*>(s), size) {}

  BytesRef(const char* begin, const char* end) noexcept
      : BytesRef(begin, static_cast<usize>(end - begin)) {}

  BytesRef(const char* cstr) noexcept
      : BytesRef(reinterpret_cast<const u8*>(cstr), std::strlen(cstr)) {}

  BytesRef(const BytesRef&) noexcept = default;
  BytesRef& operator=(const BytesRef&) noexcept = default;
  ~BytesRef() = default;

  bool starts_with(BytesRef prefix) const noexcept {
    return len() >= prefix.len() && slice(0, prefix.len()) == prefix;
  }

  BytesRef slice(usize from, usize to) const noexcept {
    assert(to <= len());
    assert(from <= to);
    return BytesRef(ptr() + from, ptr() + to);
  }

  const u8* find(u8 byte) const noexcept {
    return reinterpret_cast<const u8*>(std::memchr(ptr(), byte, len()));
  }

  const u8* begin() const noexcept {
    return data();
  }

  const u8* end() const noexcept {
    return data() + len();
  }

  operator bool() const noexcept {
    return m_data != nullptr;
  }

  bool empty() const noexcept {
    return len() == 0;
  }

  const u8* ptr() const noexcept {
    return data();
  }

  const char* char_ptr() const noexcept {
    return reinterpret_cast<const char*>(ptr());
  }

  const u8* data() const noexcept {
    return m_data;
  }

  usize size() const noexcept {
    return len();
  }

  usize len() const noexcept {
    return m_size;
  }

  bool operator==(const BytesRef rhs) const noexcept {
    return len() == rhs.len() && std::memcmp(ptr(), rhs.ptr(), len()) == 0;
  }

protected:
  const u8* m_data{nullptr};
  usize m_size{0};
};

// non-owning mutable reference to bytes
class BytesRefMut {
public:
  BytesRefMut() noexcept = default;
  BytesRefMut(u8* data, usize size) noexcept : m_data(data), m_size(size) {}

  BytesRefMut(char* cstr) noexcept
      : m_data(reinterpret_cast<u8*>(cstr))
      , m_size(std::strlen(cstr)) {}

  BytesRefMut(const BytesRefMut&) noexcept = default;
  BytesRefMut& operator=(const BytesRefMut&) noexcept = default;
  ~BytesRefMut() = default;

  operator bool() const noexcept {
    return m_data != nullptr;
  }

  BytesRef as_const() const noexcept {
    return BytesRef(ptr(), len());
  }

  const u8* ptr() const noexcept {
    return m_data;
  }

  u8* ptr() noexcept {
    return m_data;
  }

  const u8* data() const noexcept {
    return ptr();
  }

  u8* data() noexcept {
    return ptr();
  }

  usize size() const noexcept {
    return len();
  }

  usize len() const noexcept {
    return m_size;
  }

protected:
  u8* m_data{nullptr};
  usize m_size{0};
};

} // namespace shar
