#include <algorithm>
#include <cstring>

#include "request.hpp"

namespace shar::rtsp {

static Request::Type parse_type(const char* begin, std::size_t size) {

#define TRY_TYPE(TYPE)\
if((size == strlen(#TYPE)) && !memcmp(begin, #TYPE, size)) {\
  return Request::Type::TYPE;\
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

Request Request::parse(const char * buffer, const std::size_t size) {

  Request request;

  const char* current = buffer;
  const char* end = current + size;

  const char* type_end = std::find(current, end, ' ');
  if (type_end == end) {
    throw std::runtime_error("Type not found");
  }

  std::size_t type_size = type_end - current;
  Request::Type type = parse_type(current, type_size);
  current += type_size+1; //Move to first symbol after space

  const char* address_end = std::find(current, end, ' ');
  if (address_end == end) {
    throw std::runtime_error("Address not found");
  }
  std::string address = std::string(current, address_end);
  current = address_end+1; //Move to first symbol after space

  const char* version_end = find_line_ending(current, end);
  if (version_end == end) {
    throw std::runtime_error("Request finishes not on Line_ending");
  }
  std::size_t version = parse_version(current, version_end-current);

  request.set_type(type);
  request.set_address(std::move(address));
  request.set_version(version);

  if (version_end + 2 == end) {
    throw std::runtime_error("Request finishes not on Line_ending");
  }
  if (version_end + 2 != find_line_ending(version_end+2,end)) {
    current = version_end + 2; //Move to first symbol after line ending

    current = parse_headers(current, end, request.m_headers);
    if (get_content_length(request.headers()) != std::nullopt) {
      request.set_body(std::string(current+2, end)); //Move to first symbol after line ending
    }
  }
  return request;
}

Request::Type Request::type() const noexcept {
  return m_type;
}

const std::string & Request::address() const noexcept {
  return m_address;
}

std::size_t Request::version() const noexcept {
  return m_version;
}

const std::vector<Header>& Request::headers() const noexcept {
  return m_headers;
}

const std::string& Request::body() const noexcept {
  return m_body;
}

void Request::set_type(Type type) {
  m_type = type;
}

void Request::set_address(std::string address) {
  m_address = std::move(address);
}

void Request::set_version(std::size_t version) {
  m_version = version;
}

void Request::add_header(std::string key, std::string value) {
  m_headers.push_back(Header(std::move(key), std::move(value)));
}

void Request::set_body(std::string body) {
  m_body = std::move(body);
}

}