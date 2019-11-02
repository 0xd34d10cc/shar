#include "response.hpp"

#include "bufwriter.hpp"
#include "error.hpp"

#include <algorithm>
#include <cassert>
#include <charconv>

namespace shar::net::rtsp {

static const auto INVALID_STATUS_CODE = std::numeric_limits<uint16_t>::max();

Response::Response(Headers headers)
    : m_status_code(INVALID_STATUS_CODE)
    , m_headers(std::move(headers)) {}

Response::Response(const ResponseBuilder &builder) : Response(builder.finish()) {}

std::optional<usize> Response::version() const noexcept {
  return m_version;
}

u16 Response::status_code() const noexcept {
  return m_status_code;
}

std::optional<Bytes> Response::reason() const noexcept {
  return m_reason;
}

Headers Response::headers() const noexcept {
  return m_headers;
}

std::optional<Bytes> Response::body() const noexcept {
  return m_body;
}

ErrorOr<usize> Response::parse(Bytes bytes) {
  const char *current = bytes.char_ptr();
  const char *begin = current;
  const char *end = begin + bytes.len();

  const char *version_end = std::find(current, end, ' ');
  auto version = parse_version(current, static_cast<usize>(version_end - current));
  TRY(version);
  if (version_end == end) {
    FAIL(Error::NotEnoughData);
  }
  m_version = *version;
  current = version_end + 1; // Move to first symbol after space

  const char *status_code_end = std::find(current, end, ' ');
  if (status_code_end == end) {
    FAIL(Error::NotEnoughData);
  }

  auto status_code = parse_status_code(current, static_cast<usize>(status_code_end - current));
  TRY(status_code);
  if (*status_code < 100 || *status_code > 600) {
    FAIL(Error::InvalidStatusCode);
  }
  m_status_code = *status_code;
  current = status_code_end + 1; // Move to first symbol after space

  auto reason_end = find_line_ending(current, static_cast<usize>(end - current));
  TRY(reason_end);
  if (*reason_end == end) {
    FAIL(Error::NotEnoughData);
  }
  m_reason = Bytes(current, *reason_end);

  if (*reason_end + 2 == end) {
    FAIL(Error::NotEnoughData);
  }

  current = *reason_end + 2;

  auto headers_len = parse_headers(current, static_cast<usize>(end - current), m_headers);
  TRY(headers_len);

  usize response_line_size = static_cast<usize>(current - begin);
  return response_line_size + *headers_len;
}

ErrorOr<usize> Response::serialize(u8 *dst, usize size) {
  assert(m_version.has_value() || m_reason.has_value() ||
         m_status_code != std::numeric_limits<u16>::max());

  BufWriter serializer(dst, size);
#define TRY_SERIALIZE(EXP) \
  if (!(EXP))              \
    FAIL(Error::NotEnoughData)
  // serialize version

  if (*m_version == 1) {
    TRY_SERIALIZE(serializer.write("RTSP/1.0"));
  } else if (*m_version == 2) {
    TRY_SERIALIZE(serializer.write("RTSP/2.0"));
  } else {
    assert(false);
  }

  TRY_SERIALIZE(serializer.write(" "));
  // serialize status code
  TRY_SERIALIZE(serializer.format(m_status_code));
  TRY_SERIALIZE(serializer.write(" "));
  // serialize reason-phrase
  TRY_SERIALIZE(serializer.write(m_reason.value()));
  // serialize end of line
  TRY_SERIALIZE(serializer.write("\r\n"));
  // serialize headers
  for (usize i = 0; i < m_headers.len; ++i) {
    TRY_SERIALIZE(serializer.write(m_headers.data[i].name));
    TRY_SERIALIZE(serializer.write(": "));
    TRY_SERIALIZE(serializer.write(m_headers.data[i].value));
    TRY_SERIALIZE(serializer.write("\r\n"));
  }

  TRY_SERIALIZE(serializer.write("\r\n"));

  if (m_body.has_value()) {
    TRY_SERIALIZE(serializer.write(m_body.value()));
  }

  return serializer.written_bytes();
#undef TRY_SERIALIZE
}

ResponseBuilder response(Headers headers) {
  return ResponseBuilder{headers};
}

ResponseBuilder::ResponseBuilder(Headers headers) : m_response(headers) {
  m_response.m_version = 2;
  m_headers = m_response.m_headers.len;
  m_response.m_headers.len = 0;
}

ResponseBuilder ResponseBuilder::with_status(u16 code, Bytes reason) {
  assert(m_response.m_status_code == INVALID_STATUS_CODE);
  assert(!m_response.m_reason.has_value());

  m_response.m_status_code = code;
  m_response.m_reason = reason;
  return *this;
}

ResponseBuilder ResponseBuilder::with_header(Header header) {
  assert(m_response.m_headers.len < m_headers);
  assert(m_response.m_headers.data);

  m_response.m_headers.data[m_response.m_headers.len] = header;
  m_response.m_headers.len++;
  return *this;
}

ResponseBuilder ResponseBuilder::with_header(Bytes name, Bytes value) {
  return with_header(Header{name, value});
}

ResponseBuilder ResponseBuilder::with_body(Bytes body) {
  assert(!m_response.m_body.has_value());
  m_response.m_body = body;
  return *this;
}

Response ResponseBuilder::finish() const {
  assert(m_response.m_version.has_value());
  assert(m_response.m_status_code != INVALID_STATUS_CODE);
  assert(m_response.m_reason.has_value());
  return m_response;
}

} // namespace shar::net::rtsp
