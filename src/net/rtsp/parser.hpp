#pragma once

#include <vector>
#include <optional>

#include "header.hpp"
#include "request_error.hpp"
#include "error_or.hpp"


namespace shar::net::rtsp {

ErrorOr<std::uint8_t>  parse_version(const char* begin, std::size_t size);
ErrorOr<Header>       parse_header(const char* begin, std::size_t size);
ErrorOr<std::size_t>  parse_headers(const char* begin, std::size_t size, Headers headers);
ErrorOr<const char *>  find_line_ending(const char* begin, std::size_t size);
ErrorOr<std::uint16_t>  parse_status_code(const char* begin, std::size_t size);
}

