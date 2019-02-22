#include <cstring>

#include <gtest/gtest.h>

#include "rtsp/response.hpp"


using namespace shar;

TEST(rtsp_response, simple_response) {
  const char* simple_response =
    "RTSP/1.0 200 OK";
  auto response = rtsp::Response::parse(simple_response, std::strlen(simple_response));
  EXPECT_EQ(response.version(), 1);
  EXPECT_EQ(response.status_code(), 200);
  EXPECT_EQ(response.reason(), "OK");
}

TEST(rtsp_response, response_without_version) {
  const char* wo_version = "451 Invalid Parameter";
  ASSERT_THROW(
    rtsp::Response::parse(wo_version, std::strlen(wo_version))
    , std::runtime_error);
}

TEST(rtsp_response, single_header) {
  const char* single_header = 
    "RTSP/1.0 200 OK\r\n"
    "CSeq: 7";
  auto response = rtsp::Response::parse(single_header, strlen(single_header));
  EXPECT_EQ(response.version(), 1);
  EXPECT_EQ(response.status_code(), 200);
  EXPECT_EQ(response.reason(), "OK");
  EXPECT_EQ(response.headers()[0], rtsp::Header("CSeq", "7"));
}