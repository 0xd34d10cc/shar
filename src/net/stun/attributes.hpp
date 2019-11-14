#pragma once

#include "bytes_ref.hpp"


namespace shar::net::stun {

// Format of STUN attributes:
//
//   0               1               2               3
//   7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |              Type           |            Length               |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  | Value(variable)                ....
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// RFC 5389 Section 15
struct Attribute {
  u16 type{ 0 };
  u16 length{ 0 };
  u8* data{ nullptr };
};

class Attributes: public BytesRefMut {
public:
  static const usize MIN_SIZE = 0;

  Attributes() noexcept = default;
  Attributes(u8* data, usize size) noexcept;
  Attributes(const Attributes&) noexcept = default;
  Attributes(Attributes&&) noexcept = default;
  Attributes& operator=(const Attributes&) noexcept = default;
  Attributes& operator=(Attributes&&) noexcept = default;
  ~Attributes() = default;

  void reset() noexcept;

  bool read(Attribute& attr) noexcept;
  bool append(Attribute attr) noexcept;

private:
  bool valid() const noexcept;

  usize m_pos{ 0 };
};

}
