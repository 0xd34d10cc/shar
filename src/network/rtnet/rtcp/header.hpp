#pragma once

#include <cstdint> // std::uint*_t
#include <cstdlib> // std::size_t

#include "slice.hpp"


namespace shar::rtcp {

enum PacketType: std::uint8_t {
  SENDER_REPORT = 200,
  RECEIVER_REPORT = 201,
  SOURCE_DESCRIPTION = 202,
  BYE = 203,
  APP = 204
};

// RTCP packet common header structure:
//
//  0               1               2               3           
//  7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// | V |P|    RC   |      PT       |             length            |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
class Header: public Slice {
public:
  static const std::size_t NWORDS = 1;
  static const std::size_t MIN_SIZE = NWORDS * sizeof(std::uint32_t);

  // Create empty (invalid) header
  Header() noexcept = default;

  // Create header from given data
  // NOTE: |size| should be greater or equal to MIN_SIZE
  // NOTE: |data| should be 4-byte aligned
  // NOTE: ownership over |data| remains on the caller side
  //       it will not be deallocated on header destruction
  Header(std::uint8_t* data, std::size_t size) noexcept;
  Header(const Header&) noexcept = default;
  Header(Header&&) noexcept = default;
  Header& operator=(const Header&) noexcept = default;
  Header& operator=(Header&&) noexcept = default;
  ~Header() = default;

  // check if header is valid
  // NOTE: if false is returned no methods except data() and size()
  //       should be called
  bool valid() const noexcept;

  // returns pointer to packet data (header included)
  const std::uint8_t* data() const noexcept;
  std::uint8_t* data() noexcept;

  // returns buffer size
  // NOTE: buffer size is not same as packet size
  //       packet size is (length() + 1) * sizeof(std::uint32_t)
  std::size_t size() const noexcept;

  // version (V): 2 bits
  //  Identifies the version of RTP, which is the same in RTCP packets
  //  as in RTP data packets.
  std::uint8_t version() const noexcept;
  void set_version(std::uint8_t version) noexcept;

  // padding (P): 1 bit
  //  If the padding bit is set, this individual RTCP packet contains
  //  some additional padding octets at the end which are not part of
  //  the control information but are included in the length field.  The
  //  last octet of the padding is a count of how many padding octets
  //  should be ignored, including itself (it will be a multiple of
  //  four).
  bool has_padding() const noexcept;
  void set_has_padding(bool has_padding) noexcept;

  // report count (RC): 5 bits
  //  The number of report blocks contained in this packet.  A
  //  value of zero is valid.
  std::uint8_t nblocks() const noexcept;
  void set_nblocks(std::uint8_t nblocks) noexcept;

  // packet type (PT): 8 bits
  //  Contains the constant to identify this as an RTCP SR|RR packet.
  std::uint8_t packet_type() const noexcept;
  void set_packet_type(std::uint8_t type) noexcept;

  // length: 16 bits
  //  The length of this RTCP packet in 32-bit words minus one,
  //  including the header and any padding.  (The offset of one makes
  //  zero a valid length and avoids a possible infinite loop in
  //  scanning a compound RTCP packet, while counting 32-bit words
  //  avoids a validity check for a multiple of 4.)
  std::uint16_t length() const noexcept;
  void set_length(std::uint16_t length) noexcept;

  // convenience function that returns (length() + 1) * sizeof(std::uint32_t)
  std::size_t packet_size() const noexcept;

  // returns next header
  // returns invalid header if this header is last
  Header next() noexcept;
};
}

