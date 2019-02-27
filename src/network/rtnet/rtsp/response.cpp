#include <charconv>

#include "response.hpp"
#include "serializer.hpp"

namespace shar::rtsp {

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
  begin = version_end + 1; //Move to first symbol after space

  const char* status_code_end = std::find(begin, end, ' ');
  if (status_code_end == end) {
    throw std::runtime_error("Status code is undefined");
  }
  std::int64_t status_code = parse_int(begin, status_code_end - begin);
  if (status_code < 100 || status_code > 600) {
    throw std::runtime_error("Invalid status code value");
  }
  begin = status_code_end + 1; //Move to first symbol after space

  const char* reason_end = find_line_ending(begin, end);
  std::string reason(begin, reason_end);

  response.set_version(version);
  response.set_status_code(static_cast<uint16_t>(status_code));
  response.set_reason(reason);


  if (reason_end + 2 != find_line_ending(reason_end + 2, end)) {

    begin = reason_end + 2; //Move to first symbol after line_ending
    begin = parse_headers(begin, end, response.m_headers);
    if (get_content_length(response.headers()) != std::nullopt) {
      response.set_body(std::string(begin + 2, end)); //Move to first symbol after line_ending
    }
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

const std::string& Response::body() const noexcept {
  return m_body;
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
  m_headers.emplace_back(Header(std::move(key), std::move(value)));
}

void Response::set_body(std::string body) {
  m_body = std::move(body);
}

static void add_to_buffer(char* buffer, std::size_t buffer_size, char* source) {

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
  TRY_SERIALIZE(serializer.append_string(m_reason.c_str()));
  //serialize end of line
  TRY_SERIALIZE(serializer.append_string("\r\n"));
  //serialize headers
  for (auto& header : m_headers) {
    TRY_SERIALIZE(serializer.append_string(header.key.c_str()));
    TRY_SERIALIZE(serializer.append_string(": "));
    TRY_SERIALIZE(serializer.append_string(header.value.c_str()));
    TRY_SERIALIZE(serializer.append_string("\r\n"));
  }
  //serialize empty line after headers
  TRY_SERIALIZE(serializer.append_string("\r\n"));
  //serialize body
  TRY_SERIALIZE(serializer.append_string(m_body.c_str()));

  return true;
#undef TRY_SERIALIZE
}

}