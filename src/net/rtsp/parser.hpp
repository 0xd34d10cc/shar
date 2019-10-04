#pragma once

#include <vector>
#include <optional>

#include "header.hpp"
#include "rtsp_error.hpp"
#include "error_or.hpp"


namespace shar::net::rtsp {

ErrorOr<u8>  parse_version(const char* begin, usize size);
ErrorOr<Header>       parse_header(const char* begin, usize size);
ErrorOr<usize>  parse_headers(const char* begin, usize size, Headers headers);
ErrorOr<const char *>  find_line_ending(const char* begin, usize size);
ErrorOr<u16>  parse_status_code(const char* begin, usize size);
}

