#include <cstring>
#include <array>

#include <gtest/gtest.h>

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
/*
TEST(rtsp_request, serialization_small_buffer) {
  std::string base_request =
    "SET_PARAMETER rtsp://example.com/media.mp4 RTSP/1.0\r\n"
    "CSeq: 10\r\n"
    "Content-length: 20\r\n"
    "Content-type: text/parameters\r\n"
    "barparam: barstuff\r\n"
    "\r\n";
  std::string body =
    "m=video 0 RTP/AVP 96\""
    "a=control : streamid=0\r\n"
    "a=range : npt=0-7.741000\r\n"
    "a=length : npt=7.741000\r\n"
    "a=rtpmap : 96 MP4V-ES / 5544\r\n"
    "a=mimetype : string; \"video/MP4V-ES\"\r\n"
    "a=AvgBitRate:integer; 304018\r\n"
    "a=StreamName:string; \"hinted video track\"\r\n"
    "m=audio 0 RTP / AVP 97\r\n"
    "a=control:streamid = 1\r\n"
    "a=range : npt = 0 - 7.712000\r\n"
    "a=length : npt = 7.712000\r\n"
    "a=rtpmap : 97 mpeg4 - generic / 32000 / 2\r\n"
    "a=mimetype : string; \"audio/mpeg4-generic\"\r\n"
    "a=AvgBitRate:integer; 65790\r\n"
    "a=StreamName:string; \"hinted audio track\"\r\n";
  auto request_with_body = base_request + body;
  auto request
    = rtsp::Request::parse(
      request_with_body.c_str(), request_with_body.size());
  std::unique_ptr<char[]> destination = std::make_unique<char[]>(10);
  bool serialize_res = request.serialize(destination.get(), 10);
  EXPECT_FALSE(serialize_res);
}*/
//TODO: add incomplete checking tests