#pragma once 

#include <vector>
#include <optional>

#include "header.hpp"


namespace shar::rtsp {

std::optional<std::uint8_t>  parse_version(const char* begin, std::size_t size);
Header       parse_header(const char* begin, std::size_t size);
std::optional<std::size_t>  parse_headers(const char* begin, std::size_t size, Headers headers);
const char*  find_line_ending(const char* begin, const char* end);
std::optional<std::size_t> get_content_length(const Headers& headers);
std::int64_t parse_int(const char* begin, std::size_t size);
}

