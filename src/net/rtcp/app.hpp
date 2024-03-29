#pragma once

#include <array>

#include "header.hpp"


namespace shar::net::rtcp {

// APP: Application-Defined RTCP Packet
//
//     0               1               2               3
//     7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |V=2|P| subtype |   PT=APP=204  |             length            |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |                           SSRC/CSRC                           |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |                          name (ASCII)                         |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |                   application-dependent data                ...
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
class App: public Header {
public:
  static const usize NWORDS = Header::NWORDS + 2;
  static const usize MIN_SIZE = NWORDS * sizeof(u32);

  App() noexcept = default;
  App(u8* data, usize size) noexcept;
  App(const App&) noexcept = default;
  App(App&&) noexcept = default;
  App& operator=(const App&) noexcept = default;
  App& operator=(App&&) noexcept = default;
  ~App() = default;

  bool valid() const noexcept;

  // SSRC: 32 bits
  //  The synchronization source identifier for the
  //  originator of this packet.
  u32 stream_id() const noexcept;
  void set_stream_id(u32 stream_id) noexcept;

  // subtype: 5 bits
  //  May be used as a subtype to allow a set of APP packets to be
  //  defined under one unique name, or for any application-dependent
  //  data.
  u8 subtype() const noexcept;
  void set_subtype(u8 subtype) noexcept;

  // name: 4 octets
  //  A name chosen by the person defining the set of APP packets to be
  //  unique with respect to other APP packets this application might
  //  receive.  The application creator might choose to use the
  //  application name, and then coordinate the allocation of subtype
  //  values to others who want to define new packet types for the
  //  application.  Alternatively, it is RECOMMENDED that others choose
  //  a name based on the entity they represent, then coordinate the use
  //  of the name within that entity.  The name is interpreted as a
  //  sequence of four ASCII characters, with uppercase and lowercase
  //  characters treated as distinct.
  std::array<u8, 4> name() const noexcept;
  void set_name(std::array<u8, 4> name) noexcept;

  // application-dependent data: variable length
  //  Application-dependent data may or may not appear in an APP packet.
  //  It is interpreted by the application and not RTP itself.  It MUST
  //  be a multiple of 32 bits long.
  u8* payload() noexcept;
  u8 payload_size() const noexcept;
};

}