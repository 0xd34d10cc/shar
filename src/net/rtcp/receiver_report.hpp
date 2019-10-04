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
  static const usize NWORDS = Header::NWORDS + 1;
  static const usize MIN_SIZE = NWORDS * sizeof(u32);

  ReceiverReport() noexcept = default;
  ReceiverReport(u8* data, usize size) noexcept;
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
  u32 stream_id() const noexcept;
  void set_stream_id(u32 stream_id) noexcept;

  // return report block by index
  // see Block class for more info
  Block block(usize index=0) noexcept;
};

}