#include <charconv>

#include "parser.hpp"
namespace shar::rtsp {

std::size_t parse_version(const char* begin, std::size_t size) {
  if ((size != 8) && !std::memcmp(begin, "RTSP/1.0", size)) {
    throw std::runtime_error("RTSP version is incorrect");
  }
  return 1;
}

Header parse_header(const char* begin, std::size_t size) {
  const char* header_begin = begin;
  const char* key_end = std::find(begin, begin + size, ':');

  if (key_end == begin) {
    throw std::runtime_error("Incorrect additional parameter");
  }
  std::string key = std::string(begin, key_end);

  header_begin = key_end;
  if (key_end != begin + size) {
    header_begin += 2; //Move to first symbol after line_ending
  }
  std::string value = std::string(header_begin, begin + size);

  return Header(key,value);
}

const char* parse_headers(const char * begin, const char * end, Headers& headers) {
  const char* header_begin = begin;
  const char* header_end;
  bool is_headers_end = false;
  do {
    header_end = find_line_ending(header_begin, end);
    if (header_end == end) {
      throw std::runtime_error("Line ending not found");
    }
    auto[key, value] = parse_header(header_begin, header_end - header_begin);
    headers.emplace_back(Header(std::move(key), std::move(value)));
    is_headers_end = header_end + 2 == find_line_ending(header_end + 2, end);
    header_begin = header_end + 2; //Move to first symbol after line_ending
  } while (!is_headers_end);
  return header_begin;
}

const char * find_line_ending(const char * begin, const char * end)
{
  const char* it = std::find(begin, end, '\r');
  if (begin == end || (it != end &&
    *it == '\r' && *(it + 1) != '\n')) {
    throw std::runtime_error("CRLF not found");
  }
  return it;
}

std::optional<std::size_t> get_content_length(const Headers & headers)
{
  std::int64_t content_length;
  for (auto& header : headers) {
    if (header.key == "Content-Length") {
      content_length = parse_int(header.value.c_str(), header.value.size());
      if (content_length < 0) {
        throw std::runtime_error("Content-length can't have negative value");
      }
      return static_cast<std::size_t>(content_length);
    }
  }
  return std::nullopt;
}

std::int64_t parse_int(const char* begin, std::size_t size) {

  std::uint16_t number;
  auto[end_ptr, ec] = std::from_chars(begin, begin + size, number);

  if (end_ptr != begin + size || ec != std::errc()) {
    throw std::runtime_error("Status code failed parse");
  }
  return number;
}

}