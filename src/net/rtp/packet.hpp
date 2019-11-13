#pragma once

#include "int.hpp"
#include "bytes_ref.hpp"


namespace shar::net::rtp {

//  The RTP header has the following format:
//   0               1               2               3
//   7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |V=2|P|X|  CC   |M|     PT      |       sequence number         |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                           timestamp                           |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |           synchronization source (SSRC) identifier            |
//  +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//  |            contributing source (CSRC) identifiers             |
//  |                             ....                              |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// The first twelve octets are present in every RTP packet, while the
// list of CSRC identifiers is present only when inserted by a mixer.
class Packet: public BytesRefMut {
public:
  static const usize MIN_SIZE = sizeof(u32) * 3;

  // Create empty (invalid) packet
  Packet() noexcept = default;

  // Create packet from given data
  // NOTE: |data| should be 4-byte aligned
  // NOTE: |size| should be greater or equal to |MIN_SIZE|
  // NOTE: ownership over |data| remains on caller side.
  //       it will not be deallocated on packet destruction
  Packet(u8* data, usize size) noexcept;
  Packet(const Packet&) noexcept = default;
  Packet& operator=(const Packet&) noexcept = default;
  ~Packet() = default;

  // check if packet is valid
  bool valid() const noexcept;

  // version (V): 2 bits
  //  This field identifies the version of RTP.  The version defined by
  //  this library is two (2).  (The value 1 is used by the first
  //  draft version of RTP and the value 0 is used by the protocol
  //  initially implemented in the "vat" audio tool.)
  u8 version() const noexcept;
  void set_version(u8 version) noexcept;

  // padding (P): 1 bit
  //  If the padding bit is set, the packet contains one or more
  //  additional padding octets at the end which are not part of the
  //  payload.  The last octet of the padding contains a count of how
  //  many padding octets should be ignored, including itself.  Padding
  //  may be needed by some encryption algorithms with fixed block sizes
  //  or for carrying several RTP packets in a lower-layer protocol data
  //  unit.
  bool has_padding() const noexcept;
  void set_has_padding(bool has_padding) noexcept;

  // extension (X): 1 bit
  //  If the extension bit is set, the fixed header will be followed by
  //  exactly one header extension
  bool has_extensions() const noexcept;
  void set_has_extensions(bool has_extensions) noexcept;

  // CSRC count (CC): 4 bits
  //  The CSRC count contains the number of CSRC identifiers that follow
  //  the fixed header.
  u8 contributors_count() const noexcept;
  void set_contributors_count(u8 cc) noexcept;

  // marker (M): 1 bit
  //  The interpretation of the marker is defined by a profile.  It is
  //  intended to allow significant events such as frame boundaries to
  //  be marked in the packet stream.  A profile MAY define additional
  //  marker bits or specify that there is no marker bit by changing the
  //  number of bits in the payload type field.
  bool marked() const noexcept;
  void set_marked(bool mark) noexcept;

  // payload type (PT): 7 bits
  //  This field identifies the format of the RTP payload and determines
  //  its interpretation by the application.
  u8 payload_type() const noexcept;
  void set_payload_type(u8 type) noexcept;

  // sequence number: 16 bits
  //  The sequence number increments by one for each RTP data packet
  //  sent, and may be used by the receiver to detect packet loss and to
  //  restore packet sequence.
  u16 sequence() const noexcept;
  void set_sequence(u16 seq) noexcept;

  // timestamp: 32 bits
  //  The timestamp reflects the sampling instant of the first octet in
  //  the RTP data packet.  The sampling instant MUST be derived from a
  //  clock that increments monotonically and linearly in time to allow
  //  synchronization and jitter calculations (see Section 6.4.1).  The
  //  resolution of the clock MUST be sufficient for the desired
  //  synchronization accuracy and for measuring packet arrival jitter
  //  (one tick per video frame is typically not sufficient).  The clock
  //  frequency is dependent on the format of data carried as payload
  //  and is specified statically in the profile or payload format
  //  specification that defines the format, or MAY be specified
  //  dynamically for payload formats defined through non-RTP means.
  u32 timestamp() const noexcept;
  void set_timestamp(u32 timestamp) noexcept;

  // SSRC: 32 bits
  //  The SSRC field identifies the synchronization source.  This
  //  identifier SHOULD be chosen randomly, with the intent that no two
  //  synchronization sources within the same RTP session will have the
  //  same SSRC identifier.
  u32 stream_id() const noexcept;
  void set_stream_id(u32 stream_id) noexcept;

  // returns pointer to array of CSRC, 0 to 15 items
  // depending on value of contributors_count()
  u32* contributors() noexcept;

  // TODO (?)
  // ExtensionsIterator extensions();

  usize payload_size() const noexcept;

  // pointer to start of payload
  u8* payload() noexcept;
};
}
