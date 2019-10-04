#pragma once

#include "header.hpp"


namespace shar::net::rtcp {


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
  static const usize NWORDS = Header::NWORDS;
  static const usize MIN_SIZE = NWORDS * sizeof(u32);

  Bye() noexcept = default;
  Bye(u8* data, usize size) noexcept;
  Bye(const Bye&) noexcept = default;
  Bye(Bye&&) noexcept = default;
  Bye& operator=(const Bye&) noexcept = default;
  Bye& operator=(Bye&&) noexcept = default;
  ~Bye() = default;

  bool valid() const noexcept;

  // return stream id by index
  // the number of ids in this message is equal to nblocks()
  u32 stream_id(usize index) noexcept;

  bool has_reason() const noexcept;
  u8* reason() noexcept;
  u8 reason_len() const noexcept;
  void set_reason_len(u8 len) noexcept;
};

}