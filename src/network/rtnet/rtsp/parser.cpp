#include <charconv>

#include "parser.hpp"
namespace shar::rtsp {

std::optional<std::uint8_t> parse_version(const char* begin, std::size_t size) {
  int i = std::memcmp(begin, "RTSP/1.0", size);
  if ((size == 8) && i == 0) {
    return 1;
  }
  if (i == 0 && size != 8) {
    return std::nullopt;
  }
  throw std::runtime_error("Incorrect protocol version");
}

Header parse_header(const char* begin, std::size_t size) {

  const char* header_begin = begin;
  const char* end = begin + size;
  const char* key_end = std::find(begin, end, ':');

  if (key_end == begin) {
    throw std::runtime_error("Header key is empty");
  }
  if (key_end == end) {
    throw std::runtime_error("Header missin ':'");
  }
  if (end - key_end <= 2) {
    throw std::runtime_error("Header value is empty");
  }
  if (key_end[1] != ' ') {
    throw std::runtime_error("Missing space after ':'");
  }

  auto key = std::string_view(begin, key_end - begin);
  auto value = std::string_view(key_end + 2, end - key_end - 2);

  return Header(key, value);
}

std::optional<std::size_t> parse_headers(const char * begin, std::size_t size, Headers headers) {

  const char* header_begin = begin;
  const char* end = begin + size;
  const char* header_end = find_line_ending(header_begin, end);
  std::size_t index = 0;

  while (header_begin != header_end) {

    if (header_end == end) {
      return std::nullopt;
    }

    auto[key, value] = parse_header(header_begin, header_end - header_begin);
    if (index == headers.len) {
      throw std::runtime_error("Too many headers");
    }
    headers.data[index] = Header(std::move(key), std::move(value));

    header_begin = header_end + 2; //Move to first symbol after line_ending
    header_end = find_line_ending(header_begin, end);
    index++;

  }

  if (header_end == end) {
    return std::nullopt;
  }

  return header_begin - begin + 2;
}

const char * find_line_ending(const char * begin, const char * end) {
  const char* it = std::find(begin, end, '\r');
  if (begin == end || (it != end &&
    *it == '\r' && *(it + 1) != '\n')) {
    throw std::runtime_error("CRLF not found");
  }
  return it;
}


std::uint16_t parse_status_code(const char* begin, std::size_t size) {

  std::uint16_t number;
  auto[end_ptr, ec] = std::from_chars(begin, begin + size, number);

  if (end_ptr != begin + size || ec != std::errc()) {
    throw std::runtime_error("Status code failed parse");
  }
  return number;
}

}