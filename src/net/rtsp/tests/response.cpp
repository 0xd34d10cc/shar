#include <array>

#include "disable_warnings_push.hpp"
#include <gtest/gtest.h>
#include "disable_warnings_pop.hpp"

#include "bytes.hpp"
#include "net/rtsp/response.hpp"


using namespace shar;
using namespace shar::net;

static void assert_fails(Bytes response_text, rtsp::Error e) {
  std::array<rtsp::Header, 16> headers;
  rtsp::Response response(rtsp::Headers{ headers.data(), headers.size() });
  auto result = response.parse(response_text);
  ASSERT_EQ(result.err(), make_error_code(e));
}

TEST(rtsp_response, simple_response) {
  Bytes simple_response =
    "RTSP/1.0 200 OK\r\n"
    "\r\n";

  std::array<rtsp::Header, 16> headers;
  rtsp::Response response(rtsp::Headers{ headers.data(), headers.size() });

  auto size = response.parse(simple_response);
  EXPECT_FALSE(size.err());
  EXPECT_EQ(*size, simple_response.len());
  EXPECT_EQ(response.m_version, 1);
  EXPECT_EQ(response.m_status_code, 200);
  EXPECT_EQ(response.m_reason, "OK");
  EXPECT_TRUE(std::all_of(headers.begin(), headers.end(),
    [](const auto header) { return header.empty(); }));
}

TEST(rtsp_response, response_without_version) {
  assert_fails("451 Invalid Parameter\r\n", rtsp::Error::InvalidProtocol);
}

TEST(rtsp_response, single_header) {
  Bytes single_header =
    "RTSP/1.0 200 OK\r\n"
    "CSeq: 7\r\n"
    "\r\n";
  std::array<rtsp::Header, 16> headers;
  rtsp::Response response(rtsp::Headers{ headers.data(), headers.size() });

  auto size = response.parse(single_header);
  EXPECT_FALSE(size.err());
  EXPECT_EQ(*size, single_header.len());
  EXPECT_EQ(response.m_version, 1);
  EXPECT_EQ(response.m_status_code, 200);
  EXPECT_EQ(response.m_reason, "OK");
  EXPECT_EQ(headers[0], rtsp::Header("CSeq", "7"));
}

TEST(rtsp_response, ends_on_single_line_end) {
  assert_fails(
    "RTSP/1.0 200 OK\r\n"
    "CSeq: 7\r\n",
    rtsp::Error::MissingCRLF
  );
}

TEST(rtsp_response, ends_wo_line_end) {
  std::array<rtsp::Header, 16> headers;
  rtsp::Response response(rtsp::Headers{ headers.data(), headers.size() });
  auto size = response.parse(
    "RTSP/1.0 200 OK\r\n"
    "CSeq: 7"
  );
  EXPECT_TRUE(size.err());
  EXPECT_EQ(response.m_version, 1);
  EXPECT_EQ(response.m_status_code, 200);
  EXPECT_EQ(response.m_reason, "OK");
  EXPECT_TRUE(std::all_of(headers.begin(), headers.end(),
    [](const auto header) { return header.empty(); }));
}

TEST(rtsp_response, response_with_body) {
  Bytes base_response =
    "RTSP/1.0 200 OK\r\n"
    "CSeq: 2\r\n"
    "Content-Base: rtsp://example.com/media.mp4\r\n"
    "Content-Type: application/sdp\r\n"
    "Content-Length: 460\r\n"
    "\r\n";

  std::array<rtsp::Header, 16> headers;
  rtsp::Response response(rtsp::Headers{ headers.data(), headers.size() });

  auto response_size = response.parse(base_response);
  EXPECT_FALSE(response_size.err());
  EXPECT_EQ(*response_size, base_response.len());
  EXPECT_EQ(response.m_version, 1);
  EXPECT_EQ(response.m_status_code, 200);
  EXPECT_EQ(response.m_reason, "OK");
  EXPECT_EQ(headers[0], rtsp::Header("CSeq", "2"));
  EXPECT_EQ(headers[1], rtsp::Header("Content-Base", "rtsp://example.com/media.mp4"));
  EXPECT_EQ(headers[2], rtsp::Header("Content-Type", "application/sdp"));
  EXPECT_EQ(headers[3], rtsp::Header("Content-Length", "460"));
}


TEST(rtsp_response, serialization_test) {
  Bytes base_response =
    "RTSP/1.0 200 OK\r\n"
    "CSeq: 2\r\n"
    "Content-Base: rtsp://example.com/media.mp4\r\n"
    "Content-Type: application/sdp\r\n"
    "Content-Length: 460\r\n"
    "\r\n";

  std::array<rtsp::Header, 16> headers;
  rtsp::Response response(rtsp::Headers{ headers.data(), headers.size() });

  auto response_size = response.parse(base_response);
  EXPECT_FALSE(response_size.err());
  EXPECT_EQ(*response_size, base_response.len());

  u8 buffer[512];
  EXPECT_FALSE(response.serialize(buffer, 512).err());
}

TEST(rtsp_response, version_incomplete) {
  std::array<rtsp::Header, 16> headers;
  Bytes version = "RTSP";
  rtsp::Response response(rtsp::Headers{ headers.data(), headers.size() });
  EXPECT_TRUE(response.parse(version).err());
}

TEST(rtsp_response, status_code_incomplete) {
  std::array<rtsp::Header, 16> headers;
  rtsp::Response response(rtsp::Headers{ headers.data(), headers.size() });

  EXPECT_TRUE(response.parse("RTSP/1.0 2").err());
  EXPECT_EQ(response.m_version, 1);
}

TEST(rtsp_response, reason_incomplete) {
  std::array<rtsp::Header, 16> headers;
  rtsp::Response response(rtsp::Headers{ headers.data(), headers.size() });

  EXPECT_TRUE(response.parse("RTSP/1.0 200 O").err());
  EXPECT_EQ(response.m_version, 1);
  EXPECT_EQ(response.m_status_code, 200);
}

TEST(rtsp_response, status_code_invalid) {
  assert_fails("RTSP/1.0 AVS invalid status code",
               rtsp::Error::InvalidStatusCode);
}

TEST(rtsp_response, too_many_headers) {
  Bytes response_too_many_headers =
    "RTSP/1.0 200 OK\r\n"
    "CSeq: 2\r\n"
    "Content-Base: rtsp://example.com/media.mp4\r\n"
    "Content-Type: application/sdp\r\n"
    "Content-Length: 460\r\n"
    "A: A\r\n"
    "A: A\r\n"
    "A: A\r\n"
    "A: A\r\n"
    "A: A\r\n"
    "A: A\r\n"
    "A: A\r\n"
    "A: A\r\n"
    "A: A\r\n"
    "A: A\r\n"
    "A: A\r\n"
    "A: A\r\n"
    "A: A\r\n"
    "A: A\r\n"
    "A: A\r\n"
    "\r\n";

  assert_fails(response_too_many_headers, rtsp::Error::ExcessHeaders);
}
