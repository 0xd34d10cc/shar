#pragma once

#include <string>

#include "disable_warnings_push.hpp"
#include <asio/ip/address.hpp>
#include "disable_warnings_pop.hpp"


namespace shar::net {

using IpAddress = asio::ip::address;
using Port = std::uint16_t;

enum class Protocol {
  TCP,
  RTP,
  RTSP // TODO
};

class Url {
public:
  Url(Protocol proto, IpAddress ip, Port port) noexcept;
  Url(const Url&) = default;
  Url(Url&&) = default;
  Url& operator=(const Url&) = default;
  Url& operator=(Url&&) = default;
  ~Url() =default;

  static Url from_string(const std::string& str);

  Protocol protocol() const noexcept;
  IpAddress host() const noexcept;
  Port port() const noexcept;

  std::string to_string() const noexcept;

private:
  Protocol m_protocol;
  IpAddress m_host;
  Port m_port;
};

}