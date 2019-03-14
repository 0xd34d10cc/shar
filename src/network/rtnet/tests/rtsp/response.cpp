/*#include <cstring>

#include <gtest/gtest.h>

#include "rtsp/response.hpp"


using namespace shar;

TEST(rtsp_response, simple_response) {
  const char* simple_response =
    "RTSP/1.0 200 OK\r\n"
    "\r\n";
  auto response = rtsp::Response::parse(simple_response, std::strlen(simple_response));
  EXPECT_EQ(response.version(), 1);
  EXPECT_EQ(response.status_code(), 200);
  EXPECT_EQ(response.reason(), "OK");
}

TEST(rtsp_response, response_without_version) {
  const char* wo_version = "451 Invalid Parameter\r\n"
    "\r\n";
  ASSERT_THROW(
    rtsp::Response::parse(wo_version, std::strlen(wo_version))
    , std::runtime_error);
}

TEST(rtsp_response, single_header) {
  const char* single_header = 
    "RTSP/1.0 200 OK\r\n"
    "CSeq: 7\r\n"
    "\r\n";
  auto response = rtsp::Response::parse(single_header, strlen(single_header));
  EXPECT_EQ(response.version(), 1);
  EXPECT_EQ(response.status_code(), 200);
  EXPECT_EQ(response.reason(), "OK");
  EXPECT_EQ(response.headers()[0], rtsp::Header("CSeq", "7"));
}

TEST(rtsp_response, ends_on_single_line_end) {
  const char* single_header =
    "RTSP/1.0 200 OK\r\n"
    "CSeq: 7\r\n";
  ASSERT_THROW(rtsp::Response::parse(single_header, strlen(single_header)), std::runtime_error);
}

TEST(rtsp_response, ends_wo_line_end) {
  const char* single_header =
    "RTSP/1.0 200 OK\r\n"
    "CSeq: 7";
  ASSERT_THROW(rtsp::Response::parse(single_header, strlen(single_header)), std::runtime_error);
}

TEST(rtsp_response, response_with_body) {
  std::string base_response =
    "RTSP/1.0 200 OK\r\n"
    "CSeq: 2\r\n"
    "Content-Base: rtsp://example.com/media.mp4\r\n"
    "Content-Type: application/sdp\r\n"
    "Content-Length: 460\r\n"
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
  auto response_with_body = base_response + body;
  auto response 
    = rtsp::Response::parse(
      response_with_body.c_str(), response_with_body.size());
  EXPECT_EQ(response.version(), 1);
  EXPECT_EQ(response.status_code(), 200);
  EXPECT_EQ(response.reason(), "OK");
  EXPECT_EQ(response.headers()[0], rtsp::Header("CSeq", "2"));
  EXPECT_EQ(response.headers()[1], rtsp::Header("Content-Base", "rtsp://example.com/media.mp4"));
  EXPECT_EQ(response.headers()[2], rtsp::Header("Content-Type", "application/sdp"));
  EXPECT_EQ(response.headers()[3], rtsp::Header("Content-Length", "460"));
  EXPECT_EQ(response.body(), body);
}

TEST(rtsp_response, serialization_test) {
  std::string base_response =
    "RTSP/1.0 200 OK\r\n"
    "CSeq: 2\r\n"
    "Content-Base: rtsp://example.com/media.mp4\r\n"
    "Content-Type: application/sdp\r\n"
    "Content-Length: 460\r\n"
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
  auto response_with_body = base_response + body;
  auto response
    = rtsp::Response::parse(
      response_with_body.c_str(), response_with_body.size());
  std::unique_ptr<char[]> destination = std::make_unique<char[]>(response_with_body.size()+1);
  response.serialize(destination.get(), response_with_body.size()+1);
  EXPECT_EQ(response_with_body
    , std::string(destination.get(), destination.get() + response_with_body.size()));
}*/