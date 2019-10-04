#pragma once

#include "int.hpp"


namespace shar::net::rtp {

// Fragment of NAL
// 	0               1               2               3
// 	7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0
// 	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// 	| FU indicator  |   FU header   |                             |
// 	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                             |
// 	|                                                             |
// 	|                         FU payload                          |
// 	|                                                             |
// 	|                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// 	|                               :...OPTIONAL RTP padding      |
// 	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
class Fragment {
public:
  static const usize MIN_SIZE = 2;

  // indicator fields
  static const u8 NRI_MASK         = 0b01100000;
  static const u8 PACKET_TYPE_MASK = 0b00011111;
  //                                             ^ forbidden bit

  // header fields
  static const u8 START_FLAG_MASK = 0b10000000;
  static const u8 END_FLAG_MASK   = 0b01000000;
  static const u8 NAL_TYPE_MASK   = 0b00011111;
  //                                              ^ reserved bit

  Fragment() noexcept = default;
  Fragment(u8* data, usize size) noexcept;
  Fragment(const Fragment&) noexcept = default;
  Fragment(Fragment&&) noexcept = default;
  Fragment& operator=(const Fragment&) noexcept = default;
  Fragment& operator=(Fragment&&) noexcept = default;
  ~Fragment() = default;

  u8* data() const noexcept;
  usize size() const noexcept;
  bool valid() const noexcept;
  operator bool() const noexcept;

  // indicator - returns fragment indicator
  // Fragment indicator has following fields:
  //
  //	+---------------+
  // 	|0|1|2|3|4|5|6|7|
  // 	+-+-+-+-+-+-+-+-+
  // 	|F|NRI|  Type   |
  // 	+---------------+
  //
  // NOTE: bits are ordered from MSB to LSB
  u8 indicator() const noexcept;

  // NRI - returns value of NRI (nal_ref_idc)
  //
  // 	A value of 0 indicates that the content of
  // 	the NAL unit is not used to reconstruct reference pictures
  // 	for inter picture prediction. Such NAL units can be
  // 	discarded without risking the integrity of the reference
  // 	pictures
  u8 nri() const noexcept;
  void set_nri(u8 nri) noexcept;

  // packet_type - returns packet type of this fragment
  // NOTE: currently only FU-A (28) is supported
  u8 packet_type() const noexcept;
  void set_packet_type(u8 type) noexcept;

  // Header - returns fragment header
  // Fragment header has following fields
  //
  //	+---------------+
  // 	|0|1|2|3|4|5|6|7|
  // 	+-+-+-+-+-+-+-+-+
  // 	|S|E|R|  Type   |
  // 	+---------------+
  //
  // NOTE: bits are ordered from MSB to LSB
  u8 header() const noexcept;

  // is_first - returns true if this fragment is first fragment of NAL unit
  //           (value of S flag from fragment header)
  bool is_first() const noexcept;
  void set_first(bool flag) noexcept;

  // is_last - returns true if this fragment is last fragment of NAL unit
  //          (value of E flag from fragment header)
  bool is_last() const noexcept;
  void set_last(bool flag) noexcept;

  // nal_type - returns type of fragmented nal unit
  u8 nal_type() const noexcept;
  void set_nal_type(u8 type) noexcept;

  // payload - returns pointer to start of payload
  u8* payload() noexcept;
  const u8* payload() const noexcept;
  usize payload_size() const noexcept;

private:
  u8* m_data{nullptr};
  usize   m_size{0};
};

}