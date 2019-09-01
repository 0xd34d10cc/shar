#pragma once

#ifdef SHAR_TEST
// TODO
#include "mocks.hpp"
#else

#include <cstdint>
#include <system_error>

#include "disable_warnings_push.hpp"
#include <asio/io_context.hpp>
#include <asio/ip/address.hpp>
#include <asio/steady_timer.hpp>
#include <asio/buffer.hpp>
#include <asio/ip/udp.hpp>
#include <asio/ip/tcp.hpp>
#include "disable_warnings_pop.hpp"


namespace shar::net {

using IpAddress = asio::ip::address;
using IPv4 = asio::ip::address_v4;
using Port = std::uint16_t;

using IOContext = asio::io_context;
using ErrorCode = std::error_code;
using Timer = asio::steady_timer;

inline auto span(const void* data, std::size_t size) { return asio::buffer(data, size); }
inline auto span(void* data, std::size_t size) { return asio::buffer(data, size); }

namespace udp {
  using Protocol = asio::ip::udp;
  using Endpoint = Protocol::endpoint;
  using Socket = Protocol::socket;
  using Resolver = Protocol::resolver;
  inline Protocol v4() { return Protocol::v4(); }
}

namespace tcp {
  using Protocol = asio::ip::tcp;
  using Endpoint = Protocol::endpoint;
  using Socket = Protocol::socket;
  using Resolver = Protocol::resolver;
  using Acceptor = Protocol::acceptor;
  inline Protocol v4() { return Protocol::v4(); }
}

}

#endif
