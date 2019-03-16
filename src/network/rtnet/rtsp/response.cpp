#include <charconv>

#include "response.hpp"
#include "serializer.hpp"

namespace shar::rtsp {

Response::Response(Headers headers) : m_headers(std::move(headers)) {}

std::optional<std::size_t> Response::parse(const char * buffer, std::size_t size)
{

  const char* current = buffer;
  const char* end = buffer + size;

  const char* version_end = std::find(current, end, ' ');
  auto version = parse_version(current, version_end - current);
  if (version_end == end || !version.has_value()) {
    return std::nullopt;
  }
  m_version = version;
  current = version_end + 1; //Move to first symbol after space

  const char* status_code_end = std::find(current, end, ' ');
  if (status_code_end == end) {
    return std::nullopt;
  }
  auto status_code = parse_status_code(current, status_code_end - current);
  if (status_code < 100 || status_code > 600) {
    throw std::runtime_error("Invalid status code value");
  }
  m_status_code = status_code;
  current = status_code_end + 1; //Move to first symbol after space

  const char* reason_end = find_line_ending(current, end);
  if (reason_end == end) {
    return std::nullopt;
  }
  m_reason = std::string_view(current, reason_end - current);

  if (reason_end + 2 == end) {
    return std::nullopt;
  }

  current = reason_end + 2;

  auto headers_len = parse_headers(current, end - current, m_headers);
  if (!headers_len.has_value()) {
    return std::nullopt;
  }

  std::size_t response_line_size = current - buffer;

  return response_line_size + headers_len.value();
}

bool Response::serialize(char* destination, std::size_t size) {
  Serializer serializer(destination, size);
  //serialize version
#define TRY_SERIALIZE(EXP) if(!(EXP)) return false
  TRY_SERIALIZE(serializer.append_string("RTSP/1.0 "));
  //serialize status code
  TRY_SERIALIZE(serializer.append_number(m_status_code));
  //append space after number
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
  //serialize empty line after headers
  TRY_SERIALIZE(serializer.append_string("\r\n"));

  return true;
#undef TRY_SERIALIZE
}
}