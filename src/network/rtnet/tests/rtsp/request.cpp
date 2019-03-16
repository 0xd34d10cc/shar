#include <cstring>
#include <array>

#include "disable_warnings_push.hpp"
#include <gtest/gtest.h>
#include "disable_warnings_pop.hpp"

#include "rtsp/request.hpp"


using namespace shar;

static void assert_fails(const char* request_text) {
  std::array<rtsp::Header, 16> headers;
  rtsp::Request request(rtsp::Headers{ headers.data(), headers.size() });

  ASSERT_THROW(request.parse(request_text, std::strlen(request_text)), std::runtime_error);

}

TEST(rtsp_request, simple_request) {
  const char* example_request = "PLAY rtsp://server/path/test.mpg RTSP/1.0\r\n"
    "\r\n";
  std::array<rtsp::Header, 16> headers;
  rtsp::Request request(rtsp::Headers{ headers.data(), headers.size() });
  auto request_size = request.parse(example_request, std::strlen(example_request));
  EXPECT_TRUE(request_size.has_value());
  EXPECT_EQ(request_size.value(), strlen(example_request));
  EXPECT_EQ(request.m_type, rtsp::Request::Type::PLAY);
  EXPECT_EQ(request.m_address, "rtsp://server/path/test.mpg");
  EXPECT_EQ(request.m_version, 1);
  EXPECT_TRUE(std::all_of(headers.begin(), headers.end(),
    [](const auto header) { return header.empty(); }));
}

TEST(rtsp_request, trash_request) {
  assert_fails("dasdadasdfdcalxaa\r\n\r\n");
}

TEST(rtsp_request, empty_request) {
  const char* empty_request = "";
  std::array<rtsp::Header, 16> headers;
  rtsp::Request request(rtsp::Headers{ headers.data(), headers.size() });
  ASSERT_FALSE(request.parse(empty_request, std::strlen(empty_request)).has_value());
  EXPECT_TRUE(std::all_of(headers.begin(), headers.end(),
    [](const auto header) { return header.empty(); }));
}

TEST(rtsp_request, incorrect_type) {
  assert_fails("OPTI2ONS RTSP/1.0\r\n\r\n");
}

TEST(rtsp_request, request_without_address) {
  assert_fails("TEARDOWN RTSP/1.0\r\n\r\n");
}

TEST(rtsp_request, request_without_version) {
  assert_fails("PAUSE rtsp://address.address \r\n\r\n");
}

TEST(rtsp_request, request_with_header) {
  const char* request_with_header =
    "DESCRIBE rtsp://example.com/media.mp4 RTSP/1.0\r\n"
    "CSeq: 2\r\n"
    "\r\n";
  std::array<rtsp::Header, 16> headers;
  rtsp::Request request(rtsp::Headers{ headers.data(),headers.size() });
  auto request_size = request.parse(request_with_header, std::strlen(request_with_header));
  EXPECT_TRUE(request_size.has_value());
  EXPECT_EQ(request_size.value(), std::strlen(request_with_header));
  EXPECT_EQ(request.m_type, rtsp::Request::Type::DESCRIBE);
  EXPECT_EQ(request.m_address, "rtsp://example.com/media.mp4");
  EXPECT_EQ(request.m_version, 1);
  EXPECT_EQ(request.m_headers.data[0], rtsp::Header("CSeq", "2"));
  EXPECT_TRUE(std::all_of(headers.begin() + 1, headers.end(),
    [](const auto header) { return header.empty(); }));
}


TEST(rtsp_request, large_request) {
  const char* large_request =
    "SET_PARAMETER rtsp://example.com/media.mp4 RTSP/1.0\r\n"
    "CSeq: 10\r\n"
    "Content-length: 20\r\n"
    "Content-type: text/parameters\r\n"
    "barparam: barstuff\r\n"
    "\r\n";
  std::array<rtsp::Header, 16> headers;
  rtsp::Request request(rtsp::Headers{ headers.data(),headers.size() });
  auto request_size = request.parse(large_request, std::strlen(large_request));
  EXPECT_TRUE(request_size.has_value());
  EXPECT_EQ(request_size.value(), std::strlen(large_request));
  EXPECT_EQ(request.m_type, rtsp::Request::Type::SET_PARAMETER);
  EXPECT_EQ(request.m_address, "rtsp://example.com/media.mp4");
  EXPECT_EQ(request.m_version, 1);
  EXPECT_EQ(headers[0], rtsp::Header("CSeq", "10"));
  EXPECT_EQ(headers[1], rtsp::Header("Content-length", "20"));
  EXPECT_EQ(headers[2], rtsp::Header("Content-type", "text/parameters"));
  EXPECT_EQ(headers[3], rtsp::Header("barparam", "barstuff"));

  EXPECT_TRUE(std::all_of(headers.begin() + 4, headers.end(),
    [](const auto header) { return header.empty(); }));
}

TEST(rtsp_request, header_without_value) {
  assert_fails(
    "GET_PARAMETER rtsp://example.com/media.mp4 RTSP/1.0\r\n"
    "CSeq: 9\r\n"
    "packets_received\r\n"
    "\r\n"
  );
}

TEST(rtsp_request, request_serialization) {
  std::string base_request =
    "SET_PARAMETER rtsp://example.com/media.mp4 RTSP/1.0\r\n"
    "CSeq: 10\r\n"
    "Content-length: 20\r\n"
    "Content-type: text/parameters\r\n"
    "barparam: barstuff\r\n"
    "\r\n";
  std::array<rtsp::Header, 16> headers;
  rtsp::Request request(rtsp::Headers{ headers.data(),headers.size() });
  auto request_size = request.parse(base_request.c_str(), base_request.size());
  EXPECT_TRUE(request_size.has_value());
  EXPECT_EQ(request_size.value(), base_request.size());
  std::unique_ptr<char[]> destination = std::make_unique<char[]>(512);
  EXPECT_TRUE(request.serialize(destination.get(), 512));
}

TEST(rtsp_request, incompete_type) {
  const char* request_str = "DESCRI";
  std::array<rtsp::Header, 16> headers;
  rtsp::Request request(rtsp::Headers{ headers.data(),headers.size() });
  EXPECT_EQ(request.parse(request_str, strlen(request_str)),std::nullopt);
}

TEST(rtsp_request, incomplete_address) {
  const char* request_str = "OPTIONS rtsp://";
  std::array<rtsp::Header, 16> headers;
  rtsp::Request request(rtsp::Headers{ headers.data(),headers.size() });
  EXPECT_EQ(request.parse(request_str, strlen(request_str)), std::nullopt);
  EXPECT_EQ(request.m_type, rtsp::Request::Type::OPTIONS);
}

TEST(rtsp_request, incomplete_version) {
  const char* request_str = "PLAY rtsp://server/path/test.mpg RTS";
  std::array<rtsp::Header, 16> headers;
  rtsp::Request request(rtsp::Headers{ headers.data(),headers.size() });
  EXPECT_EQ(request.parse(request_str, strlen(request_str)), std::nullopt);
  EXPECT_EQ(request.m_type, rtsp::Request::Type::PLAY);
  EXPECT_EQ(request.m_address, "rtsp://server/path/test.mpg");
}

TEST(rtsp_request, incomplete_headers) {
  const char* request_str =
    "SET_PARAMETER rtsp://example.com/media.mp4 RTSP/1.0\r\n"
    "CSeq: 10\r\n"
    "Content-length";
  std::array<rtsp::Header, 16> headers;
  rtsp::Request request(rtsp::Headers{ headers.data(),headers.size() });
  EXPECT_EQ(request.parse(request_str, strlen(request_str)), std::nullopt);
  EXPECT_EQ(request.m_type, rtsp::Request::Type::SET_PARAMETER);
  EXPECT_EQ(request.m_address, "rtsp://example.com/media.mp4");
  EXPECT_EQ(request.m_version, 1);
  EXPECT_EQ(request.m_headers.data[0], rtsp::Header("CSeq", "10"));
}