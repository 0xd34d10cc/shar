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

  const char* begin = buffer;
  const char* end = begin + size;

  const char* type_end = std::find(begin, end, ' ');
  if (type_end == end) {
    throw std::runtime_error("Type not found");
  }

  std::size_t type_size = type_end - begin;
  Request::Type type = parse_type(begin, type_size);
  begin += type_size+1;

  const char* address_end = std::find(begin, end, ' ');
  if (address_end == end) {
    throw std::runtime_error("Address not found");
  }
  std::string address = std::string(begin, address_end);
  begin = address_end+1;

  const char* version_end = find_line_ending(begin, end);
  std::size_t version = parse_version(begin, version_end-begin);

  request.set_type(type);
  request.set_address(std::move(address));
  request.set_version(version);
  
  if (version_end != end) {
    begin = version_end + 2;

    parse_headers(begin, end, request.m_headers);
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

}