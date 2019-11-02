

#include "net/rtsp/request.hpp"

#include "bytes.hpp"
#include "net/rtsp/error.hpp"

#include <array>
#include <cstring>

#ifdef FAIL
#undef FAIL
#endif

// clang-format off
#include "disable_warnings_push.hpp"
#include <gtest/gtest.h>
#include "disable_warnings_pop.hpp"
// clang-format on

using namespace shar;
using namespace shar::net;

static void assert_fails(Bytes data, rtsp::Error e) {
  std::array<rtsp::Header, 16> headers;
  rtsp::Request request(rtsp::Headers{headers.data(), headers.size()});

  auto result = request.parse(data);
  ASSERT_EQ(result.err(), make_error_code(e));
}

TEST(rtsp_request, simple_request) {
  Bytes example_request = "PLAY rtsp://server/path/test.mpg RTSP/1.0\r\n"
                          "\r\n";
  std::array<rtsp::Header, 16> headers;
  rtsp::Request request(rtsp::Headers{headers.data(), headers.size()});

  auto size = request.parse(example_request);
  EXPECT_FALSE(size.err());
  EXPECT_EQ(*size, example_request.len());
  EXPECT_EQ(request.m_type, rtsp::Request::Type::PLAY);
  EXPECT_EQ(request.m_address, "rtsp://server/path/test.mpg");
  EXPECT_EQ(request.m_version, 1);
  EXPECT_TRUE(std::all_of(headers.begin(),
                          headers.end(),
                          [](const auto header) { return header.empty(); }));
}

TEST(rtsp_request, trash_request) {
  assert_fails("dasdadasdfdcalxaa\r\n\r\n", rtsp::Error::InvalidType);
}

TEST(rtsp_request, empty_request) {
  std::array<rtsp::Header, 16> headers;
  rtsp::Request request(rtsp::Headers{headers.data(), headers.size()});
  ASSERT_TRUE(request.parse("").err());
  EXPECT_TRUE(std::all_of(headers.begin(),
                          headers.end(),
                          [](const auto header) { return header.empty(); }));
}

TEST(rtsp_request, incorrect_type) {
  assert_fails("OPTI2ONS RTSP/1.0\r\n\r\n", rtsp::Error::InvalidType);
}

TEST(rtsp_request, request_without_address) {
  assert_fails("TEARDOWN RTSP/1.0\r\n\r\n", rtsp::Error::InvalidAddress);
}

TEST(rtsp_request, request_without_version) {
  assert_fails("PAUSE rtsp://address.address \r\n\r\n",
               rtsp::Error::NotEnoughData);
}

TEST(rtsp_request, request_with_header) {
  Bytes request_with_header =
      "DESCRIBE rtsp://example.com/media.mp4 RTSP/1.0\r\n"
      "CSeq: 2\r\n"
      "\r\n";

  std::array<rtsp::Header, 16> headers;
  rtsp::Request request(rtsp::Headers{headers.data(), headers.size()});
  auto size = request.parse(request_with_header);
  EXPECT_FALSE(size.err());
  EXPECT_EQ(*size, request_with_header.len());
  EXPECT_EQ(request.m_type, rtsp::Request::Type::DESCRIBE);
  EXPECT_EQ(request.m_address, "rtsp://example.com/media.mp4");
  EXPECT_EQ(request.m_version, 1);
  EXPECT_EQ(request.m_headers.data[0], rtsp::Header("CSeq", "2"));
  EXPECT_TRUE(std::all_of(headers.begin() + 1,
                          headers.end(),
                          [](const auto header) { return header.empty(); }));
}

TEST(rtsp_request, large_request) {
  Bytes large_request =
      "SET_PARAMETER rtsp://example.com/media.mp4 RTSP/1.0\r\n"
      "CSeq: 10\r\n"
      "Content-length: 20\r\n"
      "Content-type: text/parameters\r\n"
      "barparam: barstuff\r\n"
      "\r\n";

  std::array<rtsp::Header, 16> headers;
  rtsp::Request request(rtsp::Headers{headers.data(), headers.size()});

  auto size = request.parse(large_request);
  EXPECT_FALSE(size.err());
  EXPECT_EQ(*size, large_request.len());
  EXPECT_EQ(request.m_type, rtsp::Request::Type::SET_PARAMETER);
  EXPECT_EQ(request.m_address, "rtsp://example.com/media.mp4");
  EXPECT_EQ(request.m_version, 1);
  EXPECT_EQ(headers[0], rtsp::Header("CSeq", "10"));
  EXPECT_EQ(headers[1], rtsp::Header("Content-length", "20"));
  EXPECT_EQ(headers[2], rtsp::Header("Content-type", "text/parameters"));
  EXPECT_EQ(headers[3], rtsp::Header("barparam", "barstuff"));

  EXPECT_TRUE(std::all_of(headers.begin() + 4,
                          headers.end(),
                          [](const auto header) { return header.empty(); }));
}

TEST(rtsp_request, header_without_value) {
  assert_fails("GET_PARAMETER rtsp://example.com/media.mp4 RTSP/1.0\r\n"
               "CSeq: 9\r\n"
               "packets_received\r\n"
               "\r\n",
               rtsp::Error::InvalidHeader);
}

TEST(rtsp_request, request_serialization) {
  Bytes base_request = "SET_PARAMETER rtsp://example.com/media.mp4 RTSP/1.0\r\n"
                       "CSeq: 10\r\n"
                       "Content-length: 20\r\n"
                       "Content-type: text/parameters\r\n"
                       "barparam: barstuff\r\n"
                       "\r\n";

  std::array<rtsp::Header, 16> headers;
  rtsp::Request request(rtsp::Headers{headers.data(), headers.size()});
  auto size = request.parse(base_request);
  EXPECT_FALSE(size.err());
  EXPECT_EQ(*size, base_request.len());

  u8 buffer[512];
  EXPECT_FALSE(request.serialize(buffer, 512).err());
}

TEST(rtsp_request, incompete_type) {
  std::array<rtsp::Header, 16> headers;
  rtsp::Request request(rtsp::Headers{headers.data(), headers.size()});
  EXPECT_EQ(request.parse("DESCRI").err(),
            make_error_code(rtsp::Error::NotEnoughData));
}

TEST(rtsp_request, incomplete_address) {
  Bytes data = "OPTIONS rtsp://";
  std::array<rtsp::Header, 16> headers;
  rtsp::Request request(rtsp::Headers{headers.data(), headers.size()});
  EXPECT_EQ(request.parse(data).err(),
            make_error_code(rtsp::Error::NotEnoughData));
  EXPECT_EQ(request.m_type, rtsp::Request::Type::OPTIONS);
}

TEST(rtsp_request, incomplete_version) {
  Bytes data = "PLAY rtsp://server/path/test.mpg RTS";
  std::array<rtsp::Header, 16> headers;
  rtsp::Request request(rtsp::Headers{headers.data(), headers.size()});
  EXPECT_EQ(request.parse(data).err(),
            make_error_code(rtsp::Error::NotEnoughData));
  EXPECT_EQ(request.m_type, rtsp::Request::Type::PLAY);
  EXPECT_EQ(request.m_address, "rtsp://server/path/test.mpg");
}

TEST(rtsp_request, incomplete_headers) {
  Bytes data = "SET_PARAMETER rtsp://example.com/media.mp4 RTSP/1.0\r\n"
               "CSeq: 10\r\n"
               "Content-length";

  std::array<rtsp::Header, 16> headers;
  rtsp::Request request(rtsp::Headers{headers.data(), headers.size()});
  EXPECT_EQ(request.parse(data).err(),
            make_error_code(rtsp::Error::NotEnoughData));
  EXPECT_EQ(request.m_type, rtsp::Request::Type::SET_PARAMETER);
  EXPECT_EQ(request.m_address, "rtsp://example.com/media.mp4");
  EXPECT_EQ(request.m_version, 1);
  EXPECT_EQ(request.m_headers.data[0], rtsp::Header("CSeq", "10"));
}
