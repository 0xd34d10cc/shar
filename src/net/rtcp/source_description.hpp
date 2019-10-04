#pragma once

#include "header.hpp"
#include "source_items.hpp"


namespace shar::net::rtcp {


// SDES: Source Description RTCP Packet
//
//         0                   1                   2                   3
//         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// header |V=2|P|    SC   |  PT=SDES=202  |             length            |
//        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// chunk  |                          SSRC/CSRC_1                          |
//   1    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                           SDES items                          |
//        |                              ...                              |
//        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// chunk  |                          SSRC/CSRC_2                          |
//   2    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                           SDES items                          |
//        |                              ...                              |
//        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
class SourceDescription: public Header {
public:
  static const usize NWORDS = Header::NWORDS;
  static const usize MIN_SIZE = NWORDS * sizeof(u32);

  SourceDescription() noexcept = default;
  SourceDescription(u8* data, usize size);
  SourceDescription(const SourceDescription&) noexcept = default;
  SourceDescription(SourceDescription&&) noexcept = default;
  SourceDescription& operator=(const SourceDescription&) noexcept = default;
  SourceDescription& operator=(SourceDescription&&) noexcept = default;
  ~SourceDescription() = default;

  bool valid() const noexcept;

  SourceItems items() noexcept;
};

}