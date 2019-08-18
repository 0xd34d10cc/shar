#pragma once

#include <string>
#include <vector>
#include <optional>

#include "parser.hpp"


namespace shar::net::rtsp {

struct Request {

  explicit Request(Headers headers);

  enum class Type {
    OPTIONS,
    DESCRIBE,
    SETUP,
    TEARDOWN,
    PLAY,
    PAUSE,
    GET_PARAMETER,
    SET_PARAMETER,
    REDIRECT,
    ANNOUNCE,
    RECORD
  };

  std::optional<std::size_t> parse(const char* buffer, std::size_t size);

  bool serialize(char* destination, std::size_t);

  std::optional<Type> m_type;
  std::optional<std::string_view> m_address;
  std::optional<std::uint8_t> m_version;
  Headers m_headers;
};

}