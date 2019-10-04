#pragma once

#include <optional>

#include "bytes.hpp"
#include "parser.hpp"


namespace shar::net::rtsp {

class ResponseBuilder;

//Response = Status - Line;
//* (general - header;
//  | response - header;
//  | entity - header);
//  CRLF
//  [message - body];

//Status - Line = RTSP - Version SP Status - Code SP Reason - Phrase CRLF
class Response {
public:
  explicit Response(Headers headers);
  Response(const ResponseBuilder& builder);

  ErrorOr<usize> parse(Bytes bytes);
  ErrorOr<usize> serialize(unsigned char* destination, usize size);

  friend class ResponseBuilder;

  std::optional<usize> version() const noexcept;
  u16 status_code() const noexcept;
  std::optional<Bytes> reason() const noexcept;

  Headers headers() const noexcept;
  std::optional<Bytes> body() const noexcept;

private:
  std::optional<usize> m_version;
  u16 m_status_code;
  std::optional<Bytes> m_reason; // Reason-Phrase

  Headers m_headers;
  std::optional<Bytes> m_body;
};

ResponseBuilder response(Headers headers);

class ResponseBuilder {
public:
  explicit ResponseBuilder(Headers headers);
  ResponseBuilder with_status(u16 code, Bytes reason);
  ResponseBuilder with_header(Header header);
  ResponseBuilder with_header(Bytes name, Bytes value);
  ResponseBuilder with_body(Bytes body);

  Response finish() const;

private:
  Response m_response;
  usize m_headers{ 0 };
};

}