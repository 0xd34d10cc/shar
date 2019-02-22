#include <vector>

#include "header.hpp"

namespace shar::rtsp {

std::size_t parse_version(const char* begin, std::size_t size);
Header parse_header(const char* begin, std::size_t size);
void   parse_headers(const char* begin
  , const char* end
  , std::vector<Header>& headers);
const char* find_line_ending(const char* begin, const char* end);

}

