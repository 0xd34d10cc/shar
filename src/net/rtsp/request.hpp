#pragma once

#include <optional>

#include "bytes.hpp"
#include "int.hpp"
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

  ErrorOr<usize> parse(Bytes bytes);

  // returns false if destination buffer overflows
  ErrorOr<bool> serialize(unsigned char* destination, std::size_t);

  std::optional<Type> m_type;
  std::optional<Bytes> m_address;
  std::optional<std::uint8_t> m_version;

  Headers m_headers;
};

}