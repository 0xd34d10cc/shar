#include <charconv>
#include <stdexcept>
#include <cassert>

#include "response.hpp"
#include "serializer.hpp"


namespace shar::net::rtsp {

Response::Response(Headers headers)
  : m_headers(std::move(headers))
  , m_status_code(std::numeric_limits<uint16_t>::max())
{}

ErrorOr<std::size_t> Response::parse(Bytes bytes) {
  const char* current = bytes.char_ptr();
  const char* begin = current;
  const char* end = begin + bytes.len();

  const char* version_end = std::find(current, end, ' ');
  auto version = parse_version(current, version_end - current);
  TRY(version);
  if (version_end == end) {
    FAIL(Error::NotEnoughData);
  }
  m_version = *version;
  current = version_end + 1; //Move to first symbol after space

  const char* status_code_end = std::find(current, end, ' ');
  if (status_code_end == end) {
    FAIL(Error::NotEnoughData);
  }

  auto status_code = parse_status_code(current, status_code_end - current);
  TRY(status_code);
  if (*status_code < 100 || *status_code > 600) {
    FAIL(Error::InvalidStatusCode);
  }
  m_status_code = *status_code;
  current = status_code_end + 1; //Move to first symbol after space

  auto reason_end = find_line_ending(current, end - current);
  TRY(reason_end);
  if (*reason_end == end) {
    FAIL(Error::NotEnoughData);
  }
  m_reason = Bytes(current, *reason_end - current);

  if (*reason_end + 2 == end) {
    FAIL(Error::NotEnoughData);
  }

  current = *reason_end + 2;

  auto headers_len = parse_headers(current, end - current, m_headers);
  TRY(headers_len);

  std::size_t response_line_size = current - begin;
  return response_line_size + *headers_len;
}

ErrorOr<std::size_t> Response::serialize(unsigned char* dst, std::size_t size) {
  assert(m_version.has_value() || m_reason.has_value() ||
         m_status_code != std::numeric_limits<std::uint16_t>::max());

  Serializer serializer(reinterpret_cast<char*>(dst), size);
#define TRY_SERIALIZE(EXP) if(!(EXP)) FAIL(Error::NotEnoughData);
  //serialize version

  if (*m_version == 1) {
    TRY_SERIALIZE(serializer.write("RTSP/1.0"));
  }
  else if (*m_version == 2) {
    TRY_SERIALIZE(serializer.write("RTSP/2.0"));
  }
  else {
    assert(false);
  }

  TRY_SERIALIZE(serializer.write(" "));
  //serialize status code
  TRY_SERIALIZE(serializer.format(m_status_code));
  TRY_SERIALIZE(serializer.write(" "));
  //serialize reason-phrase
  TRY_SERIALIZE(serializer.write(m_reason.value()));
  //serialize end of line
  TRY_SERIALIZE(serializer.write("\r\n"));
  //serialize headers
  for (std::size_t i = 0; i < m_headers.len; ++i) {
    TRY_SERIALIZE(serializer.write(m_headers.data[i].name));
    TRY_SERIALIZE(serializer.write(": "));
    TRY_SERIALIZE(serializer.write(m_headers.data[i].value));
    TRY_SERIALIZE(serializer.write("\r\n"));
  }

  TRY_SERIALIZE(serializer.write("\r\n"));

  if (m_body.has_value()) {
    TRY_SERIALIZE(serializer.write(m_body.value()));
  }

  return serializer.written_bytes();
#undef TRY_SERIALIZE
}
}