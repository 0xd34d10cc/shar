#include <charconv>
#include <stdexcept>

#include "response.hpp"
#include "serializer.hpp"


namespace shar::net::rtsp {

#define FAIL(code) return make_error_code(code)

Response::Response(Headers headers)
  : m_headers(std::move(headers))
  , m_status_code(std::numeric_limits<uint16_t>::max()){}

ErrorOr<std::size_t> Response::parse(const char * buffer, std::size_t size)
{

  const char* current = buffer;
  const char* end = buffer + size;

  const char* version_end = std::find(current, end, ' ');
  auto version = parse_version(current, version_end - current);
  if (version.err()) {
    return version.err();
  }
  if (version_end == end) {
    FAIL(Error::notEnoughData);
  }
  m_version = *version;
  current = version_end + 1; //Move to first symbol after space

  const char* status_code_end = std::find(current, end, ' ');
  if (status_code_end == end) {
    FAIL(Error::notEnoughData);
  }
  auto status_code = parse_status_code(current, status_code_end - current);
  if (status_code.err()) {
    return status_code.err();
  }
  if (*status_code < 100 || *status_code > 600) {
    FAIL(Error::invalidStatusCode);
  }
  m_status_code = *status_code;
  current = status_code_end + 1; //Move to first symbol after space

  auto reason_end = find_line_ending(current, end - current);
  if (reason_end.err()) {
    return reason_end.err();
  }
  if (*reason_end == end) {
    FAIL(Error::notEnoughData);
  }
  m_reason = std::string_view(current, *reason_end - current);

  if (*reason_end + 2 == end) {
    FAIL(Error::notEnoughData);
  }

  current = *reason_end + 2;

  auto headers_len = parse_headers(current, end - current, m_headers);
  if (headers_len.err()) {
    return headers_len.err();
  }

  std::size_t response_line_size = current - buffer;

  return response_line_size + *headers_len;
}

ErrorOr<std::size_t> Response::serialize(char* destination, std::size_t size) {
  if (!m_version.has_value() || !m_reason.has_value() ||
       m_status_code == std::numeric_limits<std::uint16_t>::max()) {
    FAIL(Error::unitializedField);
  }
  Serializer serializer(destination, size);
#define TRY_SERIALIZE(EXP) if(!(EXP)) FAIL(Error::notEnoughData);
  //serialize version
  TRY_SERIALIZE(serializer.append_string("RTSP/1.0"));
  TRY_SERIALIZE(serializer.append_string(" "));
  //serialize status code
  TRY_SERIALIZE(serializer.append_number(m_status_code));
  TRY_SERIALIZE(serializer.append_string(" "));
  //serialize reason-phrase
  TRY_SERIALIZE(serializer.append_string(m_reason.value()));
  //serialize end of line
  TRY_SERIALIZE(serializer.append_string("\r\n"));
  //serialize headers
  for (std::size_t i = 0; i < m_headers.len; ++i) {
    TRY_SERIALIZE(serializer.append_string(m_headers.data[i].key));
    TRY_SERIALIZE(serializer.append_string(": "));
    TRY_SERIALIZE(serializer.append_string(m_headers.data[i].value));
    TRY_SERIALIZE(serializer.append_string("\r\n"));
  }
  if (m_body.has_value()) {
    TRY_SERIALIZE(serializer.append_string(m_body.value()));
    TRY_SERIALIZE(serializer.append_string("\r\n"));
  }
  //serialize empty line after headers
  TRY_SERIALIZE(serializer.append_string("\r\n"));

  return serializer.written_bytes();
#undef TRY_SERIALIZE
}
}