#include <cassert>
#include <iostream>

#include "disable_warnings_push.hpp"


extern "C" {
#include <libavcodec/avcodec.h>
}

#include "disable_warnings_pop.hpp"

#include "decoder.hpp"


namespace {


std::unique_ptr<uint8_t[]> yuv420p_to_bgra(const std::uint8_t* ys,
                                           const std::uint8_t* us,
                                           const std::uint8_t* vs,
                                           std::size_t height, std::size_t width,
                                           std::size_t y_pad, std::size_t uv_pad) {
  std::size_t y_width = width + y_pad;
  std::size_t u_width = width / 2 + uv_pad;
  std::size_t i       = 0;

  auto bgra = std::make_unique<uint8_t[]>(height * width * 4);

  for (std::size_t line = 0; line < height; ++line) {
    for (std::size_t coll = 0; coll < width; ++coll) {

      uint8_t y = ys[line * y_width + coll];
      uint8_t u = us[(line / 2) * u_width + (coll / 2)];
      uint8_t v = vs[(line / 2) * u_width + (coll / 2)];

      int c = y - 16;
      int d = u - 128;
      int e = v - 128;

      uint8_t r = static_cast<uint8_t>(clamp((298 * c + 409 * e + 128) >> 8, 0, 255));
      uint8_t g = static_cast<uint8_t>(clamp((298 * c - 100 * d - 208 * e + 128) >> 8, 0, 255));
      uint8_t b = static_cast<uint8_t>(clamp((298 * c + 516 * d + 128) >> 8, 0, 255));

      bgra[i + 0] = b;
      bgra[i + 1] = g;
      bgra[i + 2] = r;
      bgra[i + 3] = 0;

      i += 4;
    }
  }

  return bgra;
}


AVPixelFormat get_format(AVCodecContext* /*ctx*/, const enum AVPixelFormat* pix_fmts) {
  const enum AVPixelFormat* p;
  for (p = pix_fmts; *p != -1; p++) {
    if (*p == AV_PIX_FMT_YUV420P) {
      return *p;
    }
  }

  std::cerr << "YUV420 is unsupported for this decoder" << std::endl;
  return AV_PIX_FMT_NONE;
}

}

namespace shar::codecs::ffmpeg {

Decoder::Decoder()
    : m_context(nullptr)
    , m_decoder(nullptr) {
  m_decoder = avcodec_find_decoder_by_name("h264");
  m_context = avcodec_alloc_context3(m_decoder);

  assert(m_decoder);
  assert(m_context);
  std::fill_n(reinterpret_cast<char*>(m_context), sizeof(AVCodecContext), 0);


  m_context->pix_fmt    = AV_PIX_FMT_YUV420P;
  m_context->width      = 1920;
  m_context->height     = 1080;
  m_context->max_pixels = 1920 * 1080;
  m_context->get_format = get_format;
  // "unknown"
  m_context->sample_aspect_ratio.num = 16;
  m_context->sample_aspect_ratio.den = 9;

  // FIXME
  m_context->get_buffer2 = avcodec_default_get_buffer2;

  AVDictionary* opts = nullptr;
  if (avcodec_open2(m_context, m_decoder, &opts) < 0) {
    assert(false);
  }
}

Decoder::~Decoder() {
  avcodec_free_context(&m_context);
  m_context = nullptr;
  // why there is no encoder_free function in libavcodec?
  m_decoder = nullptr;

}

Image Decoder::decode(shar::Packet packet) {
  AVPacket av_packet;
  std::fill_n(reinterpret_cast<char*>(&av_packet), sizeof(av_packet), 0);
  av_packet.data = packet.data();
  av_packet.size = static_cast<int>(packet.size());

  int ret = avcodec_send_packet(m_context, &av_packet);
  if (ret != 0) {
    assert(false);
  }

  AVFrame* frame = av_frame_alloc();
  ret = avcodec_receive_frame(m_context, frame);

  // not enough data to decode
  if (ret == AVERROR(EAGAIN)) {
    return Image {};
  }

  // error
  if (ret != 0) {
    assert(false);
  }

  // unexpected format
  if (frame->format != AV_PIX_FMT_YUV420P) {
    assert(false);
  }

  std::size_t height = static_cast<std::size_t>(frame->height > m_context->height ? m_context->height : frame->height);
  std::size_t width  = static_cast<std::size_t>(frame->width);
  std::size_t y_pad  = static_cast<std::size_t>(frame->linesize[0]) - width;
  std::size_t uv_pad = static_cast<std::size_t>(frame->linesize[1]) - width / 2;
  uint8_t* y = frame->data[0];
  uint8_t* u = frame->data[1];
  uint8_t* v = frame->data[2];


  auto bytes = yuv420p_to_bgra(y, u, v, height, width, y_pad, uv_pad);
  return Image {std::move(bytes), Size {height, width}};
}

}