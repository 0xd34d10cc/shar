#pragma once

#include <string>


namespace shar::net {

using Port = std::uint16_t;

enum class Protocol {
  TCP,
  RTP,
  RTSP // TODO
};

class Url {
public:
  Url(Protocol proto, std::string host, Port port) noexcept;
  Url(const Url&) = default;
  Url(Url&&) = default;
  Url& operator=(const Url&) = default;
  Url& operator=(Url&&) = default;
  ~Url() =default;

  static Url from_string(const std::string& str);

  Protocol protocol() const noexcept;
  const std::string& host() const noexcept;
  Port port() const noexcept;

  std::string to_string() const noexcept;

private:
  Protocol m_protocol;
  std::string m_host;
  Port m_port;
};

}