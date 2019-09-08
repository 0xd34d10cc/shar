#pragma once

#include <optional>
#include <random>

#include "error_or.hpp"
#include "net/types.hpp"
#include "message.hpp"


namespace shar::net::stun {


class Request {
public:
  Request();
  Request(const Request&) = default;
  Request(Request&&) = default;
  Request& operator=(const Request&) = default;
  Request& operator=(Request&&) = default;
  ~Request() = default;

  ErrorCode send(udp::Socket& socket, udp::Endpoint server_address);
  ErrorOr<udp::Endpoint> process_response(stun::Message& response);
  void reset();

private:
  bool valid() const;
  stun::Message::Transaction generate_id();

  using Rng = std::mt19937;

  Rng m_gen;
  std::optional<stun::Message::Transaction> m_id;
};

}