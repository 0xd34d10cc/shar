#include "candidate.hpp"

#include <stdexcept>

#include "net/dns.hpp"
#include "net/stun/message.hpp"
#include "net/stun/request.hpp"
#include "forwarding.hpp"

namespace shar::net::ice {

// TODO: add timeout
// TODO: use ErrorCode instead of exceptions
static Candidate get_external_candidate(udp::Socket& socket) {
  auto server_addr = dns::resolve("stun1.l.google.com", 19302);
  // FIXME: check error first
  udp::Endpoint server{*server_addr, 19302};

  stun::Request request;

  if (auto ec = request.send(socket, server)) {
    throw std::runtime_error("Failed to send stun request: " + ec.message());
  }

  std::array<u8, 2048> buffer;
  std::fill_n(buffer.data(), 2048, static_cast<u8>(0));
  udp::Endpoint responder;

  // FIXME: no timeout
  usize n = socket.receive_from(span(buffer.data(), buffer.size()), responder);
  assert(stun::is_message(buffer.data(), n));

  stun::Message response{buffer.data(), n};
  // TODO: remove this check after adding timeout
  if (responder != server) {
    throw std::runtime_error("Received response from unexpected host: " +
                             responder.address().to_string());
  }

  auto endpoint = request.process_response(response);
  if (auto ec = endpoint.err()) {
    throw std::runtime_error("Failed to process response: " + ec.message());
  }


  return Candidate{CandidateType::Punched,
                   endpoint->address(),
                   endpoint->port()};
}

std::vector<Candidate> gather_candidates(udp::Socket& socket, Logger& logger, ErrorCode& ec) {
  std::vector<Candidate> candidates;

  // local candidates
  auto host = host_name(ec);
  if (ec) {
    return candidates;
  }

  auto local_port = socket.local_endpoint().port();
  auto local_addresses = dns::resolve_all(host, local_port);
  if (auto e = local_addresses.err()) {
    ec = e;
    return candidates;
  }

  for (const auto& addr : *local_addresses) {
    candidates.push_back(
        Candidate{CandidateType::Local, addr, local_port});
  }

  // forwarded candidates
  // TODO: remvoe exceptions from forward_port()
  bool forward_success = true;
  IpAddress wan_address;
  try {
    bool is_tcp = false;
    wan_address = forward_port(local_port, local_port, logger, is_tcp);
  } catch (const std::runtime_error& e) {
    forward_success = false;
    // logged in forward_port()
  }

  if (forward_success) {
    candidates.push_back(
        Candidate{CandidateType::Forwarded, wan_address, local_port});
  }

  // ip:port pair for UDP hole punching
  auto external_addr = get_external_candidate(socket);
  if (forward_success) {
    // TODO: check if this address can ever be valid
    candidates.push_back(
        Candidate{CandidateType::Forwarded, external_addr.ip, local_port});
  }
  candidates.push_back(external_addr);
  return candidates;
}

}