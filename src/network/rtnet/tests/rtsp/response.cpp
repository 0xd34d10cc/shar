#include <cstring>
#include <array>

#include "disable_warnings_push.hpp"
#include <gtest/gtest.h>
#include "disable_warnings_pop.hpp"

#include "rtsp/response.hpp"


using namespace shar;

static void assert_fails(std::string_view response_text) {
  std::array<rtsp::Header, 16> headers;
  rtsp::Response response(rtsp::Headers{ headers.data(), headers.size() });

  ASSERT_THROW(response.parse(response_text.data(), response_text.size()), std::runtime_error);

}

TEST(rtsp_response, simple_response) {
  std::string_view simple_response =
    "RTSP/1.0 200 OK\r\n"
    "\r\n";

  std::array<rtsp::Header, 16> headers;
  rtsp::Response response(rtsp::Headers{ headers.data(), headers.size() });

  auto response_size = 
    response.parse(simple_response.data(), simple_response.size());
  EXPECT_TRUE(response_size.has_value());
  EXPECT_EQ(response_size, simple_response.size());
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
  std::string_view single_header = 
    "RTSP/1.0 200 OK\r\n"
    "CSeq: 7\r\n"
    "\r\n";
  std::array<rtsp::Header, 16> headers;
  rtsp::Response response(rtsp::Headers{ headers.data(), headers.size() });

  auto response_size = 
    response.parse(single_header.data(), single_header.size());
  EXPECT_TRUE(response_size.has_value());
  EXPECT_EQ(response_size, single_header.size());
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
  std::string_view response_str =
    "RTSP/1.0 200 OK\r\n"
    "CSeq: 7";
  std::array<rtsp::Header, 16> headers;
  rtsp::Response response(rtsp::Headers{ headers.data(), headers.size() });

  auto response_size = 
    response.parse(response_str.data(), response_str.size());
  EXPECT_TRUE(!response_size.has_value());
  EXPECT_EQ(response.m_version, 1);
  EXPECT_EQ(response.m_status_code, 200);
  EXPECT_EQ(response.m_reason, "OK");
  EXPECT_TRUE(std::all_of(headers.begin(), headers.end(),
    [](const auto header) { return header.empty(); }));
}

TEST(rtsp_response, response_with_body) {
  std::string_view base_response =
    "RTSP/1.0 200 OK\r\n"
    "CSeq: 2\r\n"
    "Content-Base: rtsp://example.com/media.mp4\r\n"
    "Content-Type: application/sdp\r\n"
    "Content-Length: 460\r\n"
    "\r\n";
  std::array<rtsp::Header, 16> headers;
  rtsp::Response response(rtsp::Headers{ headers.data(), headers.size() });

  auto response_size = response.parse(base_response.data(), base_response.size());
  EXPECT_TRUE(response_size.has_value());
  EXPECT_EQ(response_size, base_response.size());
  EXPECT_EQ(response.m_version, 1);
  EXPECT_EQ(response.m_status_code, 200);
  EXPECT_EQ(response.m_reason, "OK");
  EXPECT_EQ(headers[0], rtsp::Header("CSeq", "2"));
  EXPECT_EQ(headers[1], rtsp::Header("Content-Base", "rtsp://example.com/media.mp4"));
  EXPECT_EQ(headers[2], rtsp::Header("Content-Type", "application/sdp"));
  EXPECT_EQ(headers[3], rtsp::Header("Content-Length", "460"));
}


TEST(rtsp_response, serialization_test) {

  std::string_view base_response =
    "RTSP/1.0 200 OK\r\n"
    "CSeq: 2\r\n"
    "Content-Base: rtsp://example.com/media.mp4\r\n"
    "Content-Type: application/sdp\r\n"
    "Content-Length: 460\r\n"
    "\r\n";
  std::array<rtsp::Header, 16> headers;
  rtsp::Response response(rtsp::Headers{ headers.data(), headers.size() });

  auto response_size = 
    response.parse(base_response.data(), base_response.size());
  EXPECT_TRUE(response_size.has_value());
  EXPECT_EQ(response_size, base_response.size());
  std::unique_ptr<char[]> destination = std::make_unique<char[]>(512);
  EXPECT_TRUE(response.serialize(destination.get(), 512));;
}

TEST(rtsp_response, version_incomplete) {
  std::array<rtsp::Header, 16> headers;
  std::string_view version = "RTSP";
  rtsp::Response response(rtsp::Headers{ headers.data(), headers.size() });
  EXPECT_FALSE(response.parse(version.data(), version.size()).has_value());
}

TEST(rtsp_response, status_code_incomplete) {
  std::array<rtsp::Header, 16> headers;
  std::string_view response_incomplete = "RTSP/1.0 2";
  rtsp::Response response(rtsp::Headers{ headers.data(), headers.size() });
  EXPECT_FALSE(response.parse(response_incomplete.data(), response_incomplete.size()).has_value());
  EXPECT_EQ(response.m_version, 1);
}

TEST(rtsp_response, reason_incomplete) {
  std::string_view response_str = "RTSP/1.0 200 O";
  std::array<rtsp::Header, 16> headers;
  rtsp::Response response(rtsp::Headers{ headers.data(), headers.size() });
  EXPECT_FALSE(response.parse(response_str.data(), response_str.size()).has_value());
  EXPECT_EQ(response.m_version, 1);
  EXPECT_EQ(response.m_status_code, 200);
}

TEST(rtsp_response, status_code_invalid) {
  assert_fails("RTSP/1.0 AVS invalid status code");
}

TEST(rtsp_response, too_many_headers) {
  std::string_view response_too_many_headers =
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
  assert_fails(response_too_many_headers);
}

TEST(rtsp_response, empty_field) {
  std::array<rtsp::Header, 16> headers;
  rtsp::Response response(rtsp::Headers{ headers.data(), headers.size() });
  auto ptr = nullptr;
  ASSERT_THROW(response.serialize(ptr, 0), std::runtime_error);
}