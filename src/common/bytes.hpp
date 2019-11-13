#pragma once

#include "bytes_ref.hpp"
#include "int.hpp"

#include <cassert> // assert
#include <memory>  // std::shared_ptr
#include <vector>

namespace shar {

// non-owning (for now) const reference to bytes
// TODO: port https://github.com/tokio-rs/bytes to C++
class Bytes {
public:
  Bytes() noexcept = default;
  Bytes(const u8* ptr, usize len) noexcept
      : m_data(std::make_shared<std::vector<u8>>(ptr, ptr + len)) {}

  Bytes(const char* p, usize l) noexcept
      : Bytes(reinterpret_cast<const u8*>(p), l) {}

  Bytes(const char* s) noexcept : Bytes(s, std::strlen(s)) {}

  Bytes(const char* start, const char* end) noexcept
      : Bytes(start, static_cast<usize>(end - start)) {}

  Bytes(const u8* start, const u8* end) noexcept
      : Bytes(start, static_cast<usize>(end - start)) {}

  BytesRef ref() const noexcept {
    return BytesRef(ptr(), len());
  }

  bool empty() const noexcept {
    return ref().empty();
  }

  bool operator==(const Bytes& bytes) const noexcept {
    return ref() == bytes.ref();
  }

  bool operator!=(const Bytes& bytes) const noexcept {
    return !(*this == bytes);
  }

  Bytes slice(usize from, usize to) const noexcept {
    assert(from <= to);
    assert(to <= len());
    return Bytes(ptr() + from, ptr() + to);
  }

  bool starts_with(BytesRef bytes) const noexcept {
    return ref().starts_with(bytes);
  }

  const u8* find(u8 byte) const noexcept {
    return ref().find(byte);
  }

  const u8* begin() const noexcept {
    return ptr();
  }

  const u8* end() const noexcept {
    return ptr() + len();
  }

  const u8* ptr() const noexcept {
    return m_data ? m_data->data() : nullptr;
  }

  usize len() const noexcept {
    return m_data ? m_data->size() : 0;
  }

  usize capacity() const noexcept {
    return m_data ? m_data->capacity() : 0;
  }

  const char* char_ptr() const {
    return reinterpret_cast<const char*>(ptr());
  }

private:
  // TODO: use custom buffer type instead of vector (for realloc)
  std::shared_ptr<std::vector<u8>> m_data;
};

} // namespace shar

inline shar::Bytes operator"" _b(const char* s) {
  return shar::Bytes{s};
}
