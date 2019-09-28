#include <charconv>
#include <stdexcept>

#include "parser.hpp"


namespace shar::net::rtsp {

#define FAIL(code) return make_error_code(code)

ErrorOr<std::uint8_t> parse_version(const char* begin, std::size_t size) {
  if (size > 8) {
    FAIL(Error::invalidProtocol);
  }
  int i = std::memcmp(begin, "RTSP/1.0", size);
  if ((size == 8) && i == 0) {
    return std::uint8_t{ 1 };
  }
  if (i == 0 && size != 8) {
    FAIL(Error::notEnoughData);
  }
  FAIL(Error::invalidProtocol);
}

ErrorOr<Header> parse_header(const char* begin, std::size_t size) {

  const char* end = begin + size;
  const char* key_end = std::find(begin, end, ':');

  if (key_end == begin || 
      key_end == end || 
      end - key_end <= 2 ||
      key_end[1] != ' ') {
    FAIL(Error::invalidHeader);
  }

  auto key = std::string_view(begin, key_end - begin);
  auto value = std::string_view(key_end + 2, end - key_end - 2);

  return Header(key, value);
}

ErrorOr<std::size_t> parse_headers(const char * begin, std::size_t size, Headers headers) {

  const char* header_begin = begin;
  const char* end = begin + size;
  auto header_end = find_line_ending(header_begin, size);
  std::size_t index = 0;

  while (!header_end.err() && header_begin != *header_end) {

    if (*header_end == end) {
      FAIL(Error::notEnoughData);
    }
    std::size_t header_size = *header_end - header_begin;

    auto header = parse_header(header_begin, header_size);
    if (header.err()) {
      return header.err();
    }
    if (index == headers.len) {
      FAIL(Error::excessHeaders);
    }
    auto [key, value] = *header;
    headers.data[index] = Header(std::move(key), std::move(value));

    header_begin = *header_end + 2; //Move to first symbol after line_ending
    header_end = find_line_ending(header_begin, end-header_begin);
    index++;

  }

  if (header_end.err()) {
    return header_end.err();
  }
  else if(*header_end == end) {
    FAIL(Error::notEnoughData);
  }

  return header_begin - begin + 2;
}

ErrorOr<const char *> find_line_ending(const char * begin, std::size_t size) {
  const char* end = begin + size;
  const char* it = std::find(begin, end, '\r');

  if (it == end - 1) {
    FAIL(Error::notEnoughData);
  }
  if (begin == end || (it != end &&
    *it == '\r' && *(it + 1) != '\n')) {
    FAIL(Error::missingCRLF);
  }
  return it;
}

ErrorOr<std::uint16_t> parse_status_code(const char* begin, std::size_t size) {

  std::uint16_t number;
  auto[end_ptr, ec] = std::from_chars(begin, begin + size, number);

  if (end_ptr != begin + size || ec != std::errc()) {
    FAIL(Error::invalidStatusCode);
  }
  return number;
}

}