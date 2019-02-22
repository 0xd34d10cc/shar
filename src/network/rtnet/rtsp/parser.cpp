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
    header_begin += 2;
  }
  std::string value = std::string(header_begin, begin + size);

  return Header(key,value);
}

void parse_headers(const char * begin, const char * end, std::vector<Header>& headers) {
  const char* header_begin = begin;
  const char* header_end;
  do {
    header_end = find_line_ending(header_begin, end);
    auto[key, value] = parse_header(header_begin, header_end - header_begin);
    headers.emplace_back(Header(std::move(key), std::move(value)));
    if (header_end != end) {
      header_begin = header_end + 2;
    }
  } while (header_end != end);

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

}