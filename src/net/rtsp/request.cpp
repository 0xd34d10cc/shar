#include <algorithm>
#include <cstring>
#include <cassert>
#include <cmath>
#include <stdexcept>

#include "request.hpp"
#include "serializer.hpp"


namespace shar::net::rtsp {

Request::Request(Headers headers) : m_headers(std::move(headers)) {}

static std::optional<Request::Type> parse_type(const char* begin, std::size_t size) {

  int i = 0;
  std::size_t len = 0;

#define TRY_TYPE(TYPE)\
len = std::string_view(#TYPE).size();\
if(size<=len){\
i = std::memcmp(begin, #TYPE, size); \
if (size == len && i == 0) {\
return Request::Type::TYPE;\
}\
if (i == 0 && size != len) {\
return std::nullopt;\
}\
}

  TRY_TYPE(OPTIONS);
  TRY_TYPE(DESCRIBE);
  TRY_TYPE(SETUP);
  TRY_TYPE(TEARDOWN);
  TRY_TYPE(PLAY);
  TRY_TYPE(PAUSE);
  TRY_TYPE(GET_PARAMETER);
  TRY_TYPE(SET_PARAMETER);
  TRY_TYPE(REDIRECT);
  TRY_TYPE(ANNOUNCE);
  TRY_TYPE(RECORD);
  throw std::runtime_error("Invalid message type");
#undef TRY_TYPE
}

static bool is_valid_address(std::string_view address) {

  for (auto c : address) {
    switch (c) {
      case '\r':
      case '\n':
      case ' ':
        return false;
      default:
        break;
    }
  }

  return true;
}

std::optional<std::size_t> Request::parse(const char * buffer, const std::size_t size) {

  const char* current = buffer;
  const char* end = current + size;

  const char* type_end = std::find(current, end, ' ');

  std::size_t type_size = type_end - current;
  auto type = parse_type(current, type_size);
  if (!type.has_value() || type_end == end) {
    return std::nullopt;
  }
  m_type = type;
  current += type_size + 1; //Move to first symbol after space

  const char* address_end = std::find(current, end, ' ');
  if (address_end - current >= 512) {
    throw std::runtime_error("Address is too big");
  }
  if (!is_valid_address(std::string_view(current, address_end - current))) {
    throw std::runtime_error("Address contains invalid characters");
  }
  if (address_end == end) {
    return std::nullopt;
  }
  m_address = std::string_view(current, address_end-current);
  current = address_end + 1; //Move to first symbol after space

  auto version_end = find_line_ending(current, end-current);
  if (!version_end.has_value()) {
    return std::nullopt;
  }
  auto version = parse_version(current, version_end.value() - current);
  if (version_end != end && !version.has_value()) {
    throw std::runtime_error("Protocol version is invalid");
  }
  if (!version.has_value() || version_end == end) {
    return std::nullopt;
  }
  m_version = version;
  //Check for requests ending without line ending
  if (version_end.value() + 2 == end) {
    return std::nullopt;
  }

  current = version_end.value() + 2; //Move to first symbol after line ending

  auto headers_len = parse_headers(current, end-current, m_headers);
  if (!headers_len.has_value()) {
    return std::nullopt;
  }

  std::size_t request_line_size = current - buffer;

  return request_line_size + headers_len.value();
}

static char* type_to_string(Request::Type type) {

  switch (type) {
    case Request::Type::OPTIONS: return "OPTIONS";
    case Request::Type::DESCRIBE: return "DESCRIBE";
    case Request::Type::SETUP: return "SETUP";
    case Request::Type::TEARDOWN: return "TEARDOWN";
    case Request::Type::PLAY: return "PLAY";
    case Request::Type::PAUSE: return "PAUSE";
    case Request::Type::GET_PARAMETER: return "GET_PARAMETER";
    case Request::Type::SET_PARAMETER: return "SET_PARAMETER";
    case Request::Type::REDIRECT: return "REDIRECT";
    case Request::Type::ANNOUNCE: return "ANNOUNCE";
    case Request::Type::RECORD: return "RECORD";
    default: assert(false); return "";
  }

}

bool Request::serialize(char* destination, std::size_t size) {
  if (!m_type.has_value() || !m_address.has_value() || !m_version.has_value()) {
    throw std::runtime_error("One of fields is not initialized");
  }

  Serializer serializer(destination, size);
#define TRY_SERIALIZE(EXP) if(!(EXP)) return false
  //serialize type
  TRY_SERIALIZE(serializer.append_string(type_to_string(m_type.value())));
  TRY_SERIALIZE(serializer.append_string(" "));
  //serialize address
  TRY_SERIALIZE(serializer.append_string(m_address.value()));
  TRY_SERIALIZE(serializer.append_string(" "));
  //serialize version
  TRY_SERIALIZE(serializer.append_string("RTSP/1.0\r\n"));
  //serialize headers
  for (std::size_t i = 0; i < m_headers.len; ++i) {
    TRY_SERIALIZE(serializer.append_string(m_headers.data[i].key));
    TRY_SERIALIZE(serializer.append_string(": "));
    TRY_SERIALIZE(serializer.append_string(m_headers.data[i].value));
    TRY_SERIALIZE(serializer.append_string("\r\n"));
  }
  //serialize empty line after headers
  TRY_SERIALIZE(serializer.append_string("\r\n"));
#undef TRY_SERIALIZE
  return true;
}

}