#include <cstring>
#include <array>

#include "disable_warnings_push.hpp"
#include <gtest/gtest.h>
#include "disable_warnings_pop.hpp"

#include "rtsp/response.hpp"


using namespace shar;

static void assert_fails(const char* response_text) {
  std::array<rtsp::Header, 16> headers;
  rtsp::Response response(rtsp::Headers{ headers.data(), headers.size() });

  ASSERT_THROW(response.parse(response_text, std::strlen(response_text)), std::runtime_error);

}

TEST(rtsp_response, simple_response) {
  const char* simple_response =
    "RTSP/1.0 200 OK\r\n"
    "\r\n";

  std::array<rtsp::Header, 16> headers;
  rtsp::Response response(rtsp::Headers{ headers.data(), headers.size() });

  auto response_size = response.parse(simple_response, std::strlen(simple_response));
  EXPECT_TRUE(response_size.has_value());
  EXPECT_EQ(response_size, std::strlen(simple_response));
  EXPECT_EQ(response.m_version, 1);
  EXPECT_EQ(response.m_status_code, 200);
  EXPECT_EQ(response.m_reason, "OK");
  EXPECT_TRUE(std::all_of(headers.begin(), headers.end(),
    [](const auto header) { return header.empty(); }));
}

TEST(rtsp_response, response_without_version) {
  assert_fails("451 Invalid Parameter\r\n");
}

TEST(rtsp_response, single_header) {
  const char* single_header = 
    "RTSP/1.0 200 OK\r\n"
    "CSeq: 7\r\n"
    "\r\n";
  std::array<rtsp::Header, 16> headers;
  rtsp::Response response(rtsp::Headers{ headers.data(), headers.size() });

  auto response_size = response.parse(single_header, std::strlen(single_header));
  EXPECT_TRUE(response_size.has_value());
  EXPECT_EQ(response_size, std::strlen(single_header));
  EXPECT_EQ(response.m_version, 1);
  EXPECT_EQ(response.m_status_code, 200);
  EXPECT_EQ(response.m_reason, "OK");
  EXPECT_EQ(headers[0], rtsp::Header("CSeq", "7"));
}
TEST(rtsp_response, ends_on_single_line_end) {
  assert_fails("RTSP/1.0 200 OK\r\n"
    "CSeq: 7\r\n");
}

TEST(rtsp_response, ends_wo_line_end) {
  const char* response_str =
    "RTSP/1.0 200 OK\r\n"
    "CSeq: 7";
  std::array<rtsp::Header, 16> headers;
  rtsp::Response response(rtsp::Headers{ headers.data(), headers.size() });

  auto response_size = response.parse(response_str, std::strlen(response_str));
  EXPECT_TRUE(!response_size.has_value());
  EXPECT_EQ(response.m_version, 1);
  EXPECT_EQ(response.m_status_code, 200);
  EXPECT_EQ(response.m_reason, "OK");
  EXPECT_TRUE(std::all_of(headers.begin(), headers.end(),
    [](const auto header) { return header.empty(); }));
}

TEST(rtsp_response, response_with_body) {
  const char* base_response =
    "RTSP/1.0 200 OK\r\n"
    "CSeq: 2\r\n"
    "Content-Base: rtsp://example.com/media.mp4\r\n"
    "Content-Type: application/sdp\r\n"
    "Content-Length: 460\r\n"
    "\r\n";
  std::array<rtsp::Header, 16> headers;
  rtsp::Response response(rtsp::Headers{ headers.data(), headers.size() });

  auto response_size = response.parse(base_response, std::strlen(base_response));
  EXPECT_TRUE(response_size.has_value());
  EXPECT_EQ(response_size, std::strlen(base_response));
  EXPECT_EQ(response.m_version, 1);
  EXPECT_EQ(response.m_status_code, 200);
  EXPECT_EQ(response.m_reason, "OK");
  EXPECT_EQ(headers[0], rtsp::Header("CSeq", "2"));
  EXPECT_EQ(headers[1], rtsp::Header("Content-Base", "rtsp://example.com/media.mp4"));
  EXPECT_EQ(headers[2], rtsp::Header("Content-Type", "application/sdp"));
  EXPECT_EQ(headers[3], rtsp::Header("Content-Length", "460"));
}


TEST(rtsp_response, serialization_test) {

  const char* base_response =
    "RTSP/1.0 200 OK\r\n"
    "CSeq: 2\r\n"
    "Content-Base: rtsp://example.com/media.mp4\r\n"
    "Content-Type: application/sdp\r\n"
    "Content-Length: 460\r\n"
    "\r\n";
  std::array<rtsp::Header, 16> headers;
  rtsp::Response response(rtsp::Headers{ headers.data(), headers.size() });

  auto response_size = response.parse(base_response, std::strlen(base_response));
  EXPECT_TRUE(response_size.has_value());
  EXPECT_EQ(response_size, std::strlen(base_response));
  std::unique_ptr<char[]> destination = std::make_unique<char[]>(512);
  EXPECT_TRUE(response.serialize(destination.get(), 512));;
}

TEST(rtsp_response, version_incomplete) {
  std::array<rtsp::Header, 16> headers;
  rtsp::Response response(rtsp::Headers{ headers.data(), headers.size() });
  EXPECT_FALSE(response.parse("RTSP", std::strlen("RTSP")).has_value());
}

TEST(rtsp_response, status_code_incomplete) {
  std::array<rtsp::Header, 16> headers;
  rtsp::Response response(rtsp::Headers{ headers.data(), headers.size() });
  EXPECT_FALSE(response.parse("RTSP/1.0 2", std::strlen("RTSP/1.0 2")).has_value());
  EXPECT_EQ(response.m_version, 1);
}

TEST(rtsp_response, reason_incomplete) {
  const char* response_str = "RTSP/1.0 200 O";
  std::array<rtsp::Header, 16> headers;
  rtsp::Response response(rtsp::Headers{ headers.data(), headers.size() });
  EXPECT_FALSE(response.parse(response_str, std::strlen(response_str)).has_value());
  EXPECT_EQ(response.m_version, 1);
  EXPECT_EQ(response.m_status_code, 200);
}

TEST(rtsp_response, status_code_invalid) {
  assert_fails("RTSP/1.0 AVS invalid status code");
}