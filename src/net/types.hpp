#pragma once

#ifdef SHAR_TEST
// TODO
#include "mocks.hpp"
#else

#include "disable_warnings_push.hpp"
#include <asio/io_context.hpp>
#include <asio/ip/address.hpp>
#include <asio/steady_timer.hpp>
#include <asio/buffer.hpp>
#include <asio/ip/udp.hpp>
#include <asio/ip/tcp.hpp>
#include "disable_warnings_pop.hpp"

#include "error_or.hpp"
#include "bytes_ref.hpp"
#include "int.hpp"


namespace shar::net {

using IpAddress = asio::ip::address;
using IPv4 = asio::ip::address_v4;
using Port = u16;

using IOContext = asio::io_context;
using Timer = asio::steady_timer;

inline auto span(const void* data, usize size) { return asio::buffer(data, size); }
inline auto span(void* data, usize size) { return asio::buffer(data, size); }
inline auto span(BytesRef bytes) { return asio::buffer(bytes.data(), bytes.len()); }
inline auto span(BytesRefMut bytes) { return asio::buffer(bytes.data(), bytes.len()); }

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
