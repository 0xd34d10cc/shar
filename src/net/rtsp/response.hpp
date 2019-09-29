#pragma once

#include <string>
#include <vector>

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

  ErrorOr<std::size_t> parse(const char* buffer, std::size_t size);
  ErrorOr<std::size_t> serialize(char* destination, std::size_t size);

  std::optional<std::size_t> m_version;
  std::uint16_t  m_status_code;
  std::optional<std::string_view> m_reason; // Reason-Phrase 

  Headers m_headers;

  std::optional<std::string_view> m_body;
};

}