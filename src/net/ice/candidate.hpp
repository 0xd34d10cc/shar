#pragma once

#include <vector>

#include "net/types.hpp"
#include "common/logger.hpp"

namespace shar::net::ice {

enum class CandidateType {
	Local,     // local network address
	Forwarded, // port forwarding
	Punched,   // udp hole punching
	Proxy      // proxy address (e.g. TURN)
};

struct Candidate {
  CandidateType type;
  IpAddress ip;
  Port port;
};

std::vector<Candidate> gather_candidates(udp::Socket& socket, Logger& logger, ErrorCode& ec);

}