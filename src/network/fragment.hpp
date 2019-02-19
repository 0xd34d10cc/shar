#pragma once

#include <cstdint> // std::uint*_t
#include <cstdlib> // std::size_t


namespace shar {

// Fragment of NAL
// 	0                   1                   2                   3
// 	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// 	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// 	| FU indicator  |   FU header   |                               |
// 	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               |
// 	|                                                               |
// 	|                         FU payload                            |
// 	|                                                               |
// 	|                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// 	|                               :...OPTIONAL RTP padding        |
// 	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
class Fragment {
public:
  static const std::size_t MIN_SIZE = 2;

  // indicator fields
  static const std::uint8_t NRI_MASK         = 0b01100000;
  static const std::uint8_t PACKET_TYPE_MASK = 0b00011111;
  //                                             ^ forbidden bit

  // header fields
  static const std::uint8_t START_FLAG_MASK = 0b10000000;
  static const std::uint8_t END_FLAG_MASK   = 0b01000000;
  static const std::uint8_t NAL_TYPE_MASK   = 0b00011111;
  //                                              ^ reserved bit

  Fragment() noexcept = default;
  Fragment(std::uint8_t* data, std::size_t size) noexcept;
  Fragment(const Fragment&) noexcept = default;
  Fragment(Fragment&&) noexcept = default;
  Fragment& operator=(const Fragment&) noexcept = default;
  Fragment& operator=(Fragment&&) noexcept = default;
  ~Fragment() = default;

  std::uint8_t* data() const noexcept;
  std::size_t size() const noexcept;
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
  std::uint8_t indicator() const noexcept;

  // NRI - returns value of NRI (nal_ref_idc)
  //
  // 	A value of 0 indicates that the content of
  // 	the NAL unit is not used to reconstruct reference pictures
  // 	for inter picture prediction. Such NAL units can be
  // 	discarded without risking the integrity of the reference
  // 	pictures
  std::uint8_t nri() const noexcept;
  void set_nri(std::uint8_t nri) noexcept;

  // packet_type - returns packet type of this fragment
  // NOTE: currently only FU-A (28) is supported
  std::uint8_t packet_type() const noexcept;
  void set_packet_type(std::uint8_t type) noexcept;

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
  std::uint8_t header() const noexcept;

  // is_first - returns true if this fragment is first fragment of NAL unit
  //           (value of S flag from fragment header)
  bool is_first() const noexcept;
  void set_first(bool flag) noexcept;

  // is_last - returns true if this fragment is last fragment of NAL unit
  //          (value of E flag from fragment header)
  bool is_last() const noexcept;
  void set_last(bool flag) noexcept;

  // nal_type - returns type of fragmented nal unit
  std::uint8_t nal_type() const noexcept;
  void set_nal_type(std::uint8_t type) noexcept;

  // payload - returns pointer to start of payload
  std::uint8_t* payload() noexcept;
  const std::uint8_t* payload() const noexcept;
  std::size_t payload_size() const noexcept;

private:
  std::uint8_t* m_data{nullptr};
  std::size_t   m_size{0};
};

}