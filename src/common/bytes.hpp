#pragma once

#include <cstring> // std::strlen, std::memcmp
#include <cassert> // assert

#include "int.hpp"


namespace shar {

// non-owning (for now) non-mutable reference to bytes
// TODO: port https://github.com/tokio-rs/bytes to C++
class Bytes {
public:
  Bytes() noexcept = default;
  Bytes(const u8* p, usize l) noexcept
    : m_ptr(p)
    , m_len(l)
  {}

  Bytes(const char* p, usize l) noexcept
    : Bytes(reinterpret_cast<const u8*>(p), l)
  {}

  Bytes(const char* s) noexcept
    : Bytes(s, std::strlen(s))
  {}

  Bytes(const char* start, const char* end) noexcept
    : Bytes(start, static_cast<usize>(end - start))
  {}

  Bytes(const u8* start, const u8* end) noexcept
    : Bytes(start, static_cast<usize>(end - start))
  {}

  bool empty() const noexcept {
    return m_len == 0;
  }

  bool operator==(const Bytes& bytes) const noexcept {
    return m_len == bytes.m_len &&
           (m_ptr == bytes.m_ptr || !std::memcmp(m_ptr, bytes.m_ptr, m_len));
  }

  bool operator!=(const Bytes& bytes) const noexcept {
    return !(*this == bytes);
  }

  Bytes slice(usize from, usize to) const noexcept {
    assert(from <= to);
    assert(to <= m_len);
    return Bytes(ptr() + from, ptr() + to);
  }

  bool starts_with(Bytes bytes) const noexcept {
    return len() >= bytes.len() && slice(0, bytes.len()) == bytes;
  }

  const u8* find(u8 byte) const {
    for (const u8* it = m_ptr; it != end(); ++it) {
      if (*it == byte) {
        return it;
      }
    }

    return nullptr;
  }

  const u8* begin() const noexcept { return ptr(); }
  const u8* end() const noexcept { return ptr() + len(); }

  const u8* ptr() const { return m_ptr; }
  usize len() const { return m_len; }

  const char* char_ptr() const {
    return reinterpret_cast<const char*>(ptr());
  }

private:
  const u8* m_ptr{ nullptr };
  usize m_len{ 0 };
};

}

inline shar::Bytes operator"" _b(const char* s) {
  return shar::Bytes{ s };
}
