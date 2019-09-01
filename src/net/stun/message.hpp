#pragma once

#include <cstdlib> // std::size_t
#include <array>

#include "slice.hpp"


namespace shar::net::stun {

// returns false if provided |data| is definitely not a valid STUN message
bool is_message(const std::uint8_t* data, std::size_t size) noexcept;

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
class Message: public Slice {
public:
  static const std::size_t MIN_SIZE = 20;
  static const std::uint32_t MAGIC = 0x2112A442;

  Message() noexcept = default;
  Message(std::uint8_t* data, std::size_t size) noexcept;
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
  std::uint16_t message_type() const noexcept;
  void set_message_type(std::uint16_t t) noexcept;

  std::uint16_t method() const noexcept;
  void set_method(std::uint16_t m) noexcept;

  // 0b00 request
  // 0b01 indication
  // 0b10 success response
  // 0b11 error response
  std::uint8_t type() const noexcept;
  void set_type(std::uint8_t t) noexcept;

  // NOTE: does not include header
  // NOTE: expected to be aligned by 4
  std::uint16_t length() const noexcept;
  void set_length(std::uint16_t l) noexcept;

  // NOTE: The magic cookie field MUST contain the fixed value 0x2112A442
  std::uint32_t cookie() const noexcept;
  void set_cookie(std::uint32_t c) noexcept;

  using Transaction = std::array<std::uint8_t, 12>;
  Transaction transaction() const noexcept;
  void set_transaction(Transaction t) noexcept;

  std::uint8_t* payload() noexcept;
  const std::uint8_t* payload() const noexcept;
  std::size_t payload_size() const noexcept;

  bool valid() const noexcept;
};

}