#include <charconv>

#include "response.hpp"


namespace shar::rtsp {

static std::uint16_t parse_status_code(const char* begin, std::size_t size) {

  std::uint16_t status_code;
  auto[end_ptr, ec] = std::from_chars(begin, begin + size, status_code);

  if (end_ptr != begin + size || ec != std::errc()) {
    throw std::runtime_error("Status code failed parse");
  }
  return status_code;
}

Response Response::parse(const char * buffer, std::size_t size)
{
  Response response;

  const char* begin = buffer;
  const char* end   = buffer + size;

  const char* version_end = std::find(begin, end, ' ');
  if (version_end == end) {
    throw std::runtime_error("Version is undefined");
  }
  std::size_t version = parse_version(begin, version_end - begin);
  begin = version_end + 1;

  const char* status_code_end = std::find(begin, end, ' ');
  if (status_code_end == end) {
    throw std::runtime_error("Status code is undefined");
  }
  std::uint16_t status_code = parse_status_code(begin, status_code_end - begin);
  begin = status_code_end + 1;

  const char* reason_end = find_line_ending(begin, end);
  std::string reason(begin, reason_end);

  response.set_version(version);
  response.set_status_code(status_code);
  response.set_reason(reason);


  if (reason_end != end) {

    begin = reason_end + 2;
    parse_headers(begin, end, response.m_headers);
  }
  return response;
}

std::size_t Response::version() const noexcept {
  return m_version;
}

std::uint16_t Response::status_code() const noexcept {
  return m_status_code;
}

const std::string & Response::reason() const noexcept {
  return m_reason;
}

const std::vector<Header>& Response::headers() const noexcept {
  return m_headers;
}

void Response::set_version(std::size_t version) {
  m_version = version;
}

void Response::set_status_code(std::uint16_t status){
  m_status_code = status;
}

void Response::set_reason(std::string reason){
  m_reason = std::move(reason);
}

void Response::add_header(std::string key, std::string value) {
  m_headers.push_back(Header(std::move(key), std::move(value)));
}

}