#include "parser.hpp"

namespace shar::rtsp {

std::size_t parse_version(const char* begin, std::size_t size) {
  if ((size != 8) && !std::memcmp(begin, "RTSP/1.0", size)) {
    throw std::runtime_error("RTSP version is incorrect");
  }
  return 1;
}

Request::Header parse_header(const char* begin, std::size_t size) {
  const char* header_begin = begin;
  const char* key_end = std::find(begin, begin + size, ':');

  if (key_end == begin) {
    throw std::runtime_error("Incorrect additional parameter");
  }
  std::string key = std::string(begin, key_end);

  header_begin = key_end;
  if (key_end != begin + size) {
    header_begin += 2;
  }
  std::string value = std::string(header_begin, begin + size);

  return std::make_pair(key, value);
}

Request::Type parse_type(const char* begin, std::size_t size) {

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

Request parse_request(const char * buffer, const std::size_t size) {

  Request request;

  const char* begin = buffer;
  const char* end = begin + size;

  const char* type_end = std::find(begin, end, ' ');
  if (type_end == end) {
    throw std::runtime_error("Type not found");
  }

  std::size_t type_size = type_end - begin;
  Request::Type type = parse_type(begin, type_size);
  begin += type_size + 1;

  const char* address_end = std::find(begin, end, ' ');
  if (address_end == end) {
    throw std::runtime_error("Address not found");
  }
  std::string address = std::string(begin, address_end);
  begin = address_end + 1;

  const char* version_end = std::find(begin, end, '\r');
  if (version_end == begin ||
    *version_end == '\r' && *(version_end + 1) != '\n') {
    throw std::runtime_error("Version not found");
  }
  std::size_t version = parse_version(begin, version_end - begin);

  request.set_type(type);
  request.set_address(std::move(address));
  request.set_version(version);

  if (version_end != end) {
    begin = version_end + 2;

    const char* header_end;
    do {
      header_end = std::find(begin, end, '\r');
      if (*header_end == 'r' && *(header_end + 1) != '\n') {
        throw std::runtime_error("Incorrect additional parameter type");
      }
      auto[key, value] = parse_header(begin, header_end - begin);
      request.add_header(std::move(key), std::move(value));
      begin = header_end + 2;
    } while (header_end != end);
  }
  return request;
}



}