#include <charconv>

#include "error.hpp"
#include "parser.hpp"


namespace shar::net::rtsp {

ErrorOr<u8> parse_version(const char* begin, usize size) {
  if (size > 8) {
    FAIL(Error::InvalidProtocol);
  }

  int i = std::memcmp(begin, "RTSP/1.0", size);
  if ((size == 8) && i == 0) {
    return u8{ 1 };
  }

  i = std::memcmp(begin, "RTSP/2.0", size);
  if ((size == 8) && i == 0) {
    return u8{ 2 };
  }

  if (i == 0 && size != 8) {
    FAIL(Error::NotEnoughData);
  }

  FAIL(Error::InvalidProtocol);
}

ErrorOr<Header> parse_header(const char* begin, usize size) {
  const char* end = begin + size;
  const char* key_end = std::find(begin, end, ':');

  if (key_end == begin ||
      key_end == end ||
      end - key_end <= 2 ||
      key_end[1] != ' ') {
    FAIL(Error::InvalidHeader);
  }

  auto key = Bytes(begin, key_end - begin);
  auto value = Bytes(key_end + 2, end - key_end - 2);

  return Header{key, value};
}

ErrorOr<usize> parse_headers(const char * begin, usize size, Headers headers) {
  const char* header_begin = begin;
  const char* end = begin + size;
  auto header_end = find_line_ending(header_begin, size);
  usize index = 0;

  while (!header_end.err() && header_begin != *header_end) {

    if (*header_end == end) {
      FAIL(Error::NotEnoughData);
    }
    usize header_size = *header_end - header_begin;

    auto header = parse_header(header_begin, header_size);
    TRY(header);
    if (index == headers.len) {
      FAIL(Error::ExcessHeaders);
    }
    auto [key, value] = *header;
    headers.data[index] = Header(std::move(key), std::move(value));

    header_begin = *header_end + 2; //Move to first symbol after line_ending
    header_end = find_line_ending(header_begin, end-header_begin);
    index++;

  }

  TRY(header_end);
  if(*header_end == end) {
    FAIL(Error::NotEnoughData);
  }

  return header_begin - begin + 2;
}

ErrorOr<const char *> find_line_ending(const char * begin, usize size) {
  const char* end = begin + size;
  const char* it = std::find(begin, end, '\r');

  if (it == end - 1) {
    FAIL(Error::NotEnoughData);
  }
  if (begin == end || (it != end &&
    *it == '\r' && *(it + 1) != '\n')) {
    FAIL(Error::MissingCRLF);
  }
  return it;
}

ErrorOr<u16> parse_status_code(const char* begin, usize size) {
  u16 number;
  auto[end_ptr, ec] = std::from_chars(begin, begin + size, number);

  if (end_ptr != begin + size || ec != std::errc()) {
    FAIL(Error::InvalidStatusCode);
  }

  return number;
}

}