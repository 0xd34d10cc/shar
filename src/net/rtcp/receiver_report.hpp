#pragma once

#include "header.hpp"
#include "block.hpp"


namespace shar::net::rtcp {

// Structure of RTCP receiver report (RR)
//
//         0               1               2               3
//         7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// header |V=2|P|    RC   |   PT=RR=201   |             length            |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                     SSRC of packet sender                     |
//        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// report |                 SSRC_1 (SSRC of first source)                 |
// block  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   1    | fraction lost |       cumulative number of packets lost       |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |           extended highest sequence number received           |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                      interarrival jitter                      |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                         last SR (LSR)                         |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                   delay since last SR (DLSR)                  |
//        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// report |                 SSRC_2 (SSRC of second source)                |
// block  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   2    :                               ...                             :
//        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//        |                  profile-specific extensions                  |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
class ReceiverReport: public Header {
public:
  static const std::size_t NWORDS = Header::NWORDS + 1;
  static const std::size_t MIN_SIZE = NWORDS * sizeof(std::uint32_t);

  ReceiverReport() noexcept = default;
  ReceiverReport(std::uint8_t* data, std::size_t size) noexcept;
  ReceiverReport(const ReceiverReport&) noexcept = default;
  ReceiverReport(ReceiverReport&&) noexcept = default;
  ReceiverReport& operator=(const ReceiverReport&) noexcept = default;
  ReceiverReport& operator=(ReceiverReport&&) noexcept = default;
  ~ReceiverReport() = default;

  // check if report is valid
  // NOTE: if false is returned no methods except data() and size()
  //       should be called
  bool valid() const noexcept;

  // SSRC: 32 bits
  //  The synchronization source identifier for the
  //  originator of this packet.
  std::uint32_t stream_id() const noexcept;
  void set_stream_id(std::uint32_t stream_id) noexcept;

  // return report block by index
  // see Block class for more info
  Block block(std::size_t index=0) noexcept;
};

}