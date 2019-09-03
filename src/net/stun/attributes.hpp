#pragma once

#include "slice.hpp"


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
  std::uint16_t type{ 0 };
  std::uint16_t length{ 0 };
  std::uint8_t* data{ nullptr };
};

class Attributes: public Slice {
public:
  static const std::size_t MIN_SIZE = 0;

  Attributes() noexcept = default;
  Attributes(std::uint8_t* data, std::size_t size) noexcept;
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

  std::size_t m_pos{ 0 };
};

}