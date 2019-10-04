#pragma once

#include <optional>

#include "bytes.hpp"
#include "parser.hpp"


namespace shar::net::rtsp {

//Response = Status - Line;
//* (general - header;
//  | response - header;
//  | entity - header);
//  CRLF
//  [message - body];

//Status - Line = RTSP - Version SP Status - Code SP Reason - Phrase CRLF
struct Response {
  explicit Response(Headers headers);

  ErrorOr<usize> parse(Bytes bytes);
  ErrorOr<usize> serialize(unsigned char* destination, std::size_t size);

  std::optional<std::size_t> m_version;
  u16 m_status_code;
  std::optional<Bytes> m_reason; // Reason-Phrase

  Headers m_headers;
  std::optional<Bytes> m_body;
};

}