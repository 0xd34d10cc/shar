#include <algorithm>
#include <cstring>
#include <cassert>
#include <cmath>

#include "request.hpp"
#include "serializer.hpp"


namespace shar::net::rtsp {

Request::Request(Headers headers) : m_headers(std::move(headers)) {}

static ErrorOr<Request::Type> parse_type(const char* begin, usize size) {
  /*
  static std::array<std::pair<BytesRef, Request::Type>, 11> types{
    {"OPTIONS"_b,  Request::Type::OPTIONS},
    {"DESCRIBE"_b, Request::Type::DESCRIBE},
    {"SETUP"_b,    Request::Type::SETUP},
    {"TEARDOWN"_b, Request::Type::TEARDOWN},
    {"PLAY"_b,     Request::Type::PLAY},
    {"PAUSE"_b,    Request::Type::PAUSE},
    {"REDIRECT"_b, Request::Type::REDIRECT},
    {"ANNOUNCE"_b, Request::Type::ANNOUNCE},
    {"RECORD"_b,   Request::Type::RECORD},
    {"GET_PARAMETER"_b, Request::Type::GET_PARAMETER},
    {"SET_PARAMETER"_b, Request::Type::SET_PARAMETER}
  };
  */

  int i = 0;
  usize len = 0;

#define TRY_TYPE(TYPE)\
len = std::strlen(#TYPE);\
if(size<=len){\
i = std::memcmp(begin, #TYPE, size); \
if (size == len && i == 0) {\
return Request::Type::TYPE;\
}\
if (i == 0 && size != len) {\
FAIL(Error::NotEnoughData);\
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
  FAIL(Error::InvalidType);
#undef TRY_TYPE
}

static bool is_valid_address(Bytes address) {
  for (u8 c : address) {
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

ErrorOr<usize> Request::parse(Bytes bytes) {
  const char* current = bytes.char_ptr();
  const char* begin = current;
  const char* end = current + bytes.len();

  const char* type_end = std::find(current, end, ' ');

  usize type_size = type_end - current;
  auto type = parse_type(current, type_size);
  TRY(type);
  if(type_end == end) {
    FAIL(Error::NotEnoughData);
  }
  m_type = *type;
  current += type_size + 1; //Move to first symbol after space

  const char* address_end = std::find(current, end, ' ');
  if (address_end - current >= 512 ||
    !is_valid_address(Bytes(current, address_end - current))) {
    FAIL(Error::InvalidAddress);
  }

  if (address_end == end) {
    FAIL(Error::NotEnoughData);
  }

  m_address = Bytes(current, address_end-current);
  current = address_end + 1; //Move to first symbol after space

  auto version_end = find_line_ending(current, end-current);
  TRY(version_end);
  auto version = parse_version(current, *version_end - current);

  TRY(version);
  if (*version_end == end) {
    FAIL(Error::NotEnoughData);
  }

  m_version = *version;

  //Check for requests ending without line ending
  if (end - *version_end <= 2) {
    FAIL(Error::NotEnoughData);
  }

  current = *version_end + 2; //Move to first symbol after line ending

  auto headers_len = parse_headers(current, end-current, m_headers);
  TRY(headers_len);

  usize request_line_size = current - begin;
  return request_line_size + *headers_len;
}

static Bytes type_to_string(Request::Type type) {
  switch (type) {
    case Request::Type::OPTIONS:
      return "OPTIONS";
    case Request::Type::DESCRIBE:
      return "DESCRIBE";
    case Request::Type::SETUP:
      return "SETUP";
    case Request::Type::TEARDOWN:
      return "TEARDOWN";
    case Request::Type::PLAY:
      return "PLAY";
    case Request::Type::PAUSE:
      return "PAUSE";
    case Request::Type::GET_PARAMETER:
      return "GET_PARAMETER";
    case Request::Type::SET_PARAMETER:
      return "SET_PARAMETER";
    case Request::Type::REDIRECT:
      return "REDIRECT";
    case Request::Type::ANNOUNCE:
      return "ANNOUNCE";
    case Request::Type::RECORD:
      return "RECORD";
    default:
      assert(false);
      return "";
  }
}

ErrorOr<bool> Request::serialize(unsigned char* dst, usize size) {
  assert(m_type.has_value() || m_address.has_value() || m_version.has_value());

  Serializer serializer(reinterpret_cast<char*>(dst), size);
#define TRY_SERIALIZE(exp) if(!(exp)) return false
  //serialize type
  TRY_SERIALIZE(serializer.write(type_to_string(m_type.value())));
  TRY_SERIALIZE(serializer.write(" "));
  //serialize address
  TRY_SERIALIZE(serializer.write(m_address.value()));
  TRY_SERIALIZE(serializer.write(" "));
  //serialize version
  TRY_SERIALIZE(serializer.write("RTSP/1.0\r\n"));
  //serialize headers
  for (usize i = 0; i < m_headers.len; ++i) {
    TRY_SERIALIZE(serializer.write(m_headers.data[i].name));
    TRY_SERIALIZE(serializer.write(": "));
    TRY_SERIALIZE(serializer.write(m_headers.data[i].value));
    TRY_SERIALIZE(serializer.write("\r\n"));
  }

  //serialize empty line after headers
  TRY_SERIALIZE(serializer.write("\r\n"));
#undef TRY_SERIALIZE
  return true;
}

}