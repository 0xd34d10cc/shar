#pragma once

#include <vector>

#include "net/types.hpp"
#include "common/logger.hpp"

namespace shar::net::ice {

//
//   To Internet
//        |
//        |  /------------  Relayed
//        | /               Address
//    +--------+
//    |        |
//    |  TURN  |
//    | Server |
//    |        |
//    +--------+
//        |
//        | /------------  Server
//        |/               Reflexive
//  +------------+         Address
//  | outer NAT  |
//  +------------+
//        |
//        | /-------------- Forwarded
//        |/                Address
//  +------------+
//  | inner NAT  |
//  +------------+
//        |
//        | /------------  Local
//        |/               Address
//    +--------+
//    |        |
//    | Agent  |
//    |        |
//    +--------+
//
enum class CandidateType {
	Local,           // port on local network interface
	Forwarded,       // port forwarded via IGD
	ServerReflexive, // NAT allocated port (obtained via STUN)
	Relayed          // TURN allocated address
};

struct Candidate {
  CandidateType type;
  IpAddress ip;
  Port port;
};

std::vector<Candidate> gather_candidates(udp::Socket& socket, Logger& logger, ErrorCode& ec);

}