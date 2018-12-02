#pragma once

#include "header.hpp"


namespace shar::rtcp {


//        0               1               2               3    
//        7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       |V=2|P|    SC   |   PT=BYE=203  |             length            |
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       |                           SSRC/CSRC                           |
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       :                              ...                              :
//       +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// (opt) |     length    |               reason for leaving            ...
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
class Bye: public Header {
public:
  static const std::size_t NWORDS = Header::NWORDS;
  static const std::size_t MIN_SIZE = NWORDS * sizeof(std::uint32_t);

  Bye() noexcept = default;
  Bye(std::uint8_t* data, std::size_t size) noexcept;
  Bye(const Bye&) noexcept = default;
  Bye(Bye&&) noexcept = default;
  Bye& operator=(const Bye&) noexcept = default;
  Bye& operator=(Bye&&) noexcept = default;
  ~Bye() = default;

  bool valid() const noexcept;

  // return pointer to start of SSRC/CSRC block.
  std::uint8_t* ids() noexcept;

  bool has_reason() const noexcept;
  std::uint8_t* reason() noexcept;
  std::uint8_t reason_len() const noexcept;
  void set_reason_len(std::uint8_t len) noexcept;
};

}