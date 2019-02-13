#include <charconv>

#include "disable_warnings_push.hpp"
#include <fmt/format.h>
#include "disable_warnings_pop.hpp"

#include "url.hpp"


namespace shar {

Url::Url(Protocol proto, IpAddress ip, Port port) noexcept
  : m_protocol(proto)
  , m_host(std::move(ip))
  , m_port(port)
{}

Protocol Url::protocol() const noexcept {
  return m_protocol;
}

IpAddress Url::host() const noexcept {
  return m_host;
}

Port Url::port() const noexcept {
  return m_port;
}

Url Url::from_string(const std::string& str) {
  // Minimal example:
  // tcp://0.0.0.0
  // 0123456789
  if (str.size() < 12) {
    throw std::runtime_error("Url is too small");
  }

  const char* begin = str.data();
  const char* end = str.data() + str.size();
  
  const auto find = [&](const char* substring, std::size_t from_index=0) {
    std::size_t index = str.find(substring, from_index);
    if (index == std::string::npos) {
      return end;
    }

    return begin + index;
  };
   
  const char* proto_end = find("://");
  const auto protocol = [&]{
    if (proto_end == end) {
      throw std::runtime_error("Missing protocol in URL");
    }

    if (std::memcmp(begin, "tcp://", 6) == 0) {
      return Protocol::TCP;
    }

    if (std::memcmp(begin, "rtp://", 6) == 0) {
      return Protocol::RTP;
    }

    if (std::memcmp(begin, "rtsp://", 7) == 0) {
      return Protocol::RTSP;
    }

    throw std::runtime_error("Unknown protocol");
  }();
   
  const char* host_end = find(":", proto_end+3 - begin); // could be |end|
  const auto host = IpAddress::from_string(std::string(proto_end+3, host_end));
  const auto port = [&] {
    if (host_end == end) {
      // select "default" port
      switch (protocol) {
        case Protocol::TCP:
          return Port(8080);
        case Protocol::RTP:
          return Port(1336);
        case Protocol::RTSP:
          return Port(1234);
      }

      assert(false);
    }
  
    // parse port
    std::uint16_t port = 0;
    auto[ptr, err] = std::from_chars(host_end+1, end, port);
    if (err != std::errc()) {
      throw std::runtime_error("Failed to parse port");
    }

    return Port(port);
  }();

  return Url(protocol, host, port);
}

static const char* to_str(Protocol protocol) {
  switch (protocol) {
    case Protocol::TCP:
      return "tcp";
    case Protocol::RTP:
      return "rtp";
    case Protocol::RTSP:
      return "rtsp";
  }

  assert(false);
}

std::string Url::to_string() const noexcept {
  return fmt::format("{}://{}:{}", to_str(protocol()), host().to_string(), port());
}

}
