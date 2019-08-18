#pragma once

#include <string>
#include <vector>

#include "parser.hpp"


namespace shar::net::rtsp {

struct Response {

  explicit Response(Headers headers);

  std::optional<std::size_t> parse(const char* buffer, std::size_t size);
  //return false if we haven't enough space in buffer
  bool serialize(char* destination, std::size_t size);

  std::optional<std::size_t> m_version;
  std::uint16_t  m_status_code;
  std::optional<std::string_view> m_reason;

  Headers m_headers;
};

}