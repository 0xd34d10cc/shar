#pragma once

#include "packet.hpp"

#include "disable_warnings_push.hpp"
#include <asio/ip/udp.hpp>
#include "disable_warnings_pop.hpp"


namespace shar::rtp {

using UDPSocket = asio::ip::udp::socket;
using Endpoint = asio::ip::udp::endpoint;

class Session {
public:
  Session(UDPSocket socket, Endpoint endpoint) noexcept;
  Session(const Session&) = delete;
  Session(Session&&) = default;
  Session& operator=(const Session&) = delete;
  Session& operator=(Session&&) = default;
  ~Session() = default;

  template <typename Handler>
  void send(Packet packet, Handler&& handler) noexcept {
    m_socket.async_send_to(boost::asio::buffer(packet.data(), packet.size()),
                           m_endpoint, std::forward<Handler>(handler));
  }

private:
  UDPSocket m_socket;
  Endpoint m_endpoint;
};

}