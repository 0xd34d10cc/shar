#include <cstring>

#include <gtest/gtest.h>

#include "rtsp/request.hpp"

using namespace shar;

TEST(rtsp_request, simple_request) {
  const char* example_request = "PLAY rtsp://server/path/test.mpg RTSP/1.0";
  rtsp::Request request = rtsp::Request::parse(example_request, std::strlen(example_request));
  EXPECT_EQ(request.type(), rtsp::Request::Type::PLAY);
  EXPECT_EQ(request.address(), "rtsp://server/path/test.mpg");
  EXPECT_EQ(request.version(), 1);
}

TEST(rtsp_request, trash_request) {
  const char* trash_request = "dasdadasdfdcalxaa";
  ASSERT_THROW(rtsp::Request::parse(trash_request, std::strlen(trash_request)), std::runtime_error);
}

TEST(rtsp_request, empty_request) {
  const char* empty_request = "";
  ASSERT_THROW(rtsp::Request::parse(empty_request, std::strlen(empty_request)), std::runtime_error);
}

TEST(rtsp_request, incorrect_type) {
  const char* empty_request = "OPTI2ONS RTSP/1.0";
  ASSERT_THROW(rtsp::Request::parse(empty_request, std::strlen(empty_request)), std::runtime_error);
}

TEST(rtsp_request, request_without_address) {
  const char* empty_request = "TEARDOWN RTSP/1.0";
  ASSERT_THROW(rtsp::Request::parse(empty_request, std::strlen(empty_request)), std::runtime_error);
}

TEST(rtsp_request, request_without_version) {
  const char* empty_request = "PAUSE RTSP/1.0";
  ASSERT_THROW(rtsp::Request::parse(empty_request, std::strlen(empty_request)), std::runtime_error);
}

TEST(rtsp_request, request_with_header) {
  const char* request_with_header = 
    "DESCRIBE rtsp://example.com/media.mp4 RTSP/1.0\r\n"
    "CSeq: 2";
  auto request = rtsp::Request::parse(request_with_header, std::strlen(request_with_header));
  EXPECT_EQ(request.type(), rtsp::Request::Type::DESCRIBE);
  EXPECT_EQ(request.address(), "rtsp://example.com/media.mp4");
  EXPECT_EQ(request.version(), 1);
  EXPECT_EQ(request.headers()[0].key, "CSeq");
  EXPECT_EQ(request.headers()[0].value, "2");
}

TEST(rtsp_request, large_request) {
  const char* large_request =
    "SET_PARAMETER rtsp://example.com/media.mp4 RTSP/1.0\r\n"
    "CSeq: 10\r\n"
    "Content-length: 20\r\n"
    "Content-type: text/parameters\r\n"
    "barparam: barstuff";
  auto request = rtsp::Request::parse(large_request, std::strlen(large_request));
  EXPECT_EQ(request.type(), rtsp::Request::Type::SET_PARAMETER);
  EXPECT_EQ(request.address(), "rtsp://example.com/media.mp4");
  EXPECT_EQ(request.version(), 1);
  auto headers = request.headers();
  EXPECT_EQ(headers[0].key, "CSeq");
  EXPECT_EQ(headers[0].value, "10");
  EXPECT_EQ(headers[1].key, "Content-length");
  EXPECT_EQ(headers[1].value, "20");
  EXPECT_EQ(headers[2].key, "Content-type");
  EXPECT_EQ(headers[2].value, "text/parameters");
  EXPECT_EQ(headers[3].key, "barparam");
  EXPECT_EQ(headers[3].value, "barstuff");
}

TEST(rtsp_request, header_without_value) {
  const char* request_without_value =
    "GET_PARAMETER rtsp://example.com/media.mp4 RTSP/1.0\r\n"
    "CSeq: 9\r\n"
    "packets_received";
  auto request = rtsp::Request::parse(request_without_value, std::strlen(request_without_value));
  EXPECT_EQ(request.type(), rtsp::Request::Type::GET_PARAMETER);
  EXPECT_EQ(request.address(), "rtsp://example.com/media.mp4");
  EXPECT_EQ(request.version(), 1);
  auto headers = request.headers();
  EXPECT_EQ(headers[0].key, "CSeq");
  EXPECT_EQ(headers[0].value, "9");
  EXPECT_EQ(headers[1].key, "packets_received");
  EXPECT_EQ(headers[1].value, "");
}