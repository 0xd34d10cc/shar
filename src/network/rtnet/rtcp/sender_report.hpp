#pragma once

#include "header.hpp"
#include "block.hpp"


namespace shar::rtcp {

// Structure of RTCP SR (sender report):
// 
//         0               1               2               3           
//         7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// header |V=2|P|    RC   |   PT=SR=200   |             length            |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                         SSRC of sender                        |
//        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// sender |              NTP timestamp, most significant word             |
// info   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |             NTP timestamp, least significant word             |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                         RTP timestamp                         |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                     sender's packet count                     |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                      sender's octet count                     |
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
class SenderReport: public Header {
public:
  static const std::size_t NWORDS = Header::NWORDS + 6;
  static const std::size_t MIN_SIZE = NWORDS * sizeof(std::uint32_t);

  // Create empty (invalid) sender report
  SenderReport() noexcept = default;

  // Create sender report from given data
  // NOTE: |size| should be greater or equal to Base::MIN_SIZE + SenderReport::MIN_SIZE
  // NOTE: |data| should be 4-byte aligned
  // NOTE: ownership over |data| remains on the caller side
  //       it will not be deallocated on header destruction
  // NOTE: |size| SHOULD be equal to Base::packet_size()
  SenderReport(std::uint8_t* data, std::size_t size) noexcept;
  SenderReport(const SenderReport&) noexcept = default;
  SenderReport(SenderReport&&) noexcept = default;
  SenderReport& operator=(const SenderReport&) noexcept = default;
  SenderReport& operator=(SenderReport&&) noexcept = default;
  ~SenderReport() = default;

  // check if report is valid
  // NOTE: if false is returned no methods except data() and size()
  //       should be called
  bool valid() const noexcept;

  // SSRC: 32 bits
  //  The synchronization source identifier for the 
  //  originator of this packet.
  std::uint32_t stream_id() const noexcept;
  void set_stream_id(std::uint32_t stream_id) noexcept;

  // NTP timestamp: 64 bits
  //  Indicates the wallclock time when this report was
  //  sent so that it may be used in combination with timestamps
  //  returned in reception reports from other receivers to measure
  //  round-trip propagation to those receivers.  Receivers should
  //  expect that the measurement accuracy of the timestamp may be
  //  limited to far less than the resolution of the NTP timestamp.  The
  //  measurement uncertainty of the timestamp is not indicated as it
  //  may not be known.  On a system that has no notion of wallclock
  //  time but does have some system-specific clock such as "system
  //  uptime", a sender MAY use that clock as a reference to calculate
  //  relative NTP timestamps.  It is important to choose a commonly
  //  used clock so that if separate implementations are used to produce
  //  the individual streams of a multimedia session, all
  //  implementations will use the same clock.  Until the year 2036,
  //  relative and absolute timestamps will differ in the high bit so
  //  (invalid) comparisons will show a large difference; by then one
  //  hopes relative timestamps will no longer be needed.  A sender that
  //  has no notion of wallclock or elapsed time MAY set the NTP
  //  timestamp to zero.
  std::uint64_t ntp_timestamp() const noexcept;
  void set_ntp_timestamp(std::uint64_t timestamp) noexcept;

  // RTP timestamp: 32 bits
  //  Corresponds to the same time as the NTP timestamp (above), but in
  //  the same units and with the same random offset as the RTP
  //  timestamps in data packets.  This correspondence may be used for
  //  intra- and inter-media synchronization for sources whose NTP
  //  timestamps are synchronized, and may be used by media-independent
  //  receivers to estimate the nominal RTP clock frequency.  Note that
  //  in most cases this timestamp will not be equal to the RTP
  //  timestamp in any adjacent data packet.  Rather, it MUST be
  //  calculated from the corresponding NTP timestamp using the
  //  relationship between the RTP timestamp counter and real time as
  //  maintained by periodically checking the wallclock time at a
  //  sampling instant.
  std::uint32_t rtp_timestamp() const noexcept;
  void set_rtp_timestamp(std::uint32_t timestamp) noexcept;

  // sender's packet count: 32 bits
  //  The total number of RTP data packets transmitted by the sender
  //  since starting transmission up until the time this SR packet was
  //  generated.  The count SHOULD be reset if the sender changes its
  //  SSRC identifier.
  std::uint32_t npackets() const noexcept;
  void set_npackets(std::uint32_t npackets) noexcept;

  // sender's octet count: 32 bits
  //  The total number of payload octets (i.e., not including header or
  //  padding) transmitted in RTP data packets by the sender since
  //  starting transmission up until the time this SR packet was
  //  generated.  The count SHOULD be reset if the sender changes its
  //  SSRC identifier.  This field can be used to estimate the average
  //  payload data rate.
  std::uint32_t nbytes() const noexcept;
  void set_nbytes(std::uint32_t nbytes) noexcept;

  // return report block by index
  // see Block class for more info
  Block block(std::size_t index=0) noexcept;
};

}