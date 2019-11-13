#pragma once

#include <cstdlib> // usize
#include <array>

#include "bytes_ref.hpp"


namespace shar::net::stun {

// returns false if provided |data| is definitely not a valid STUN message
bool is_message(const u8* data, usize size) noexcept;

// STUN message header has the following format:
//
//   0               1               2               3
//   7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |0 0|   STUN Message Type     |       Message Length            |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                        Magic Cookie                           |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                                                               |
//  |             Transaction ID(96 bits, 12 bytes)                 |
//  |                                                               |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// RFC 5389 Section 6
class Message: public BytesRefMut {
public:
  static const usize MIN_SIZE = 20;
  static const u32 MAGIC = 0x2112A442;

  Message() noexcept = default;
  Message(u8* data, usize size) noexcept;
  Message(const Message&) noexcept = default;
  Message(Message&&) noexcept = default;
  Message& operator=(const Message&) noexcept = default;
  Message& operator=(Message&&) noexcept = default;
  ~Message() = default;

  // Format of STUN Message Type field
  //                      1
  //    5  4  3  2  1  0  7  6  5  4  3  2  1  0
  //  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  //  | M| M| M| M| M| C| M| M| M| C| M| M| M| M|
  //  |11|10| 9| 8| 7| 1| 6| 5| 4| 0| 3| 2| 1| 0|
  //  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  //
  // M bits here are for method and C bits for class (type)
  u16 message_type() const noexcept;
  void set_message_type(u16 t) noexcept;

  u16 method() const noexcept;
  void set_method(u16 m) noexcept;

  // 0b00 request
  // 0b01 indication
  // 0b10 success response
  // 0b11 error response
  u8 type() const noexcept;
  void set_type(u8 t) noexcept;

  // NOTE: does not include header
  // NOTE: expected to be aligned by 4
  u16 length() const noexcept;
  void set_length(u16 l) noexcept;

  // NOTE: The magic cookie field MUST contain the fixed value 0x2112A442
  u32 cookie() const noexcept;
  void set_cookie(u32 c) noexcept;

  using Transaction = std::array<u8, 12>;
  Transaction transaction() const noexcept;
  void set_transaction(Transaction t) noexcept;

  u8* payload() noexcept;
  const u8* payload() const noexcept;
  usize payload_size() const noexcept;

  bool valid() const noexcept;
};

}
