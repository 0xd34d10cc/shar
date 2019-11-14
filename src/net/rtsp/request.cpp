#include <algorithm>
#include <cstring>
#include <cassert>

#include "bufwriter.hpp"
#include "error.hpp"
#include "request.hpp"


namespace shar::net::rtsp {

Request::Request(Headers headers) : m_headers(std::move(headers)) {}

static ErrorOr<Request::Type> parse_type(const char* begin, usize size) {
  int i = 0;
  usize len = 0;

#define TRY_TYPE(TYPE)                     \
  do {                                     \
    len = std::strlen(#TYPE);              \
    if (size <= len) {                     \
      i = std::memcmp(begin, #TYPE, size); \
      if (size == len && i == 0) {         \
        return Request::Type::TYPE;        \
      }                                    \
      if (i == 0 && size != len) {         \
        FAIL(Error::NotEnoughData);        \
      }                                    \
    }                                      \
  } while (false)

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

static bool is_valid_address(BytesRef address) {
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

ErrorOr<usize> Request::parse(BytesRef bytes) {
  const char* current = bytes.char_ptr();
  const char* begin = current;
  const char* end = current + bytes.len();

  const char* type_end = std::find(current, end, ' ');

  usize type_size = static_cast<usize>(type_end - current);
  auto type = parse_type(current, type_size);
  TRY(type);
  if(type_end == end) {
    FAIL(Error::NotEnoughData);
  }
  m_type = *type;
  current += type_size + 1; //Move to first symbol after space

  const char* address_end = std::find(current, end, ' ');
  if (address_end - current >= 512 ||
    !is_valid_address(BytesRef(current, address_end))) {
    FAIL(Error::InvalidAddress);
  }

  if (address_end == end) {
    FAIL(Error::NotEnoughData);
  }

  m_address = BytesRef(current, address_end);
  current = address_end + 1; //Move to first symbol after space

  auto version_end = find_line_ending(current, static_cast<usize>(end-current));
  TRY(version_end);
  auto version = parse_version(current, static_cast<usize>(*version_end - current));

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

  auto headers_len = parse_headers(current, static_cast<usize>(end-current), m_headers);
  TRY(headers_len);

  usize request_line_size = static_cast<usize>(current - begin);
  return request_line_size + *headers_len;
}

static BytesRef type_to_string(Request::Type type) {
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

ErrorOr<bool> Request::serialize(u8* dst, usize size) {
  assert(m_type.has_value() || m_address.has_value() || m_version.has_value());

  BufWriter serializer(dst, size);
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
