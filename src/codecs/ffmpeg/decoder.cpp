#include <cassert>

#include "disable_warnings_push.hpp"
extern "C" {
#include <libavcodec/avcodec.h>
}
#include "disable_warnings_pop.hpp"

#include "codecs/convert.hpp"
#include "codecs/ffmpeg/decoder.hpp"


static AVPixelFormat get_format(AVCodecContext* /*ctx*/, const enum AVPixelFormat* pix_fmts) {
  const enum AVPixelFormat* p;
  for (p = pix_fmts; *p != -1; p++) {
    if (*p == AV_PIX_FMT_YUV420P) {
      return *p;
    }
  }
  spdlog::get("shar_logger")->error("YUV420 pixel format is not supported for this decoder");
  return AV_PIX_FMT_NONE;
}

namespace shar::codecs::ffmpeg {

Decoder::Decoder(Logger logger)
    : m_context(nullptr)
    , m_decoder(nullptr)
    , m_logger(std::move(logger)) {
  m_decoder = avcodec_find_decoder(AV_CODEC_ID_H264);
  m_context = avcodec_alloc_context3(m_decoder);

  assert(m_decoder);
  assert(m_context);
  std::fill_n(reinterpret_cast<char*>(m_context), sizeof(AVCodecContext), 0);

  m_context->pix_fmt                 = AV_PIX_FMT_YUV420P;
  // FIXME: unhardcode
  m_context->width                   = 1920;
  m_context->height                  = 1080;
  m_context->max_pixels              = 1920 * 1080;
  m_context->get_format              = get_format;
  m_context->sample_aspect_ratio.num = 16;
  m_context->sample_aspect_ratio.den = 9;
  m_context->get_buffer2             = avcodec_default_get_buffer2;

  AVDictionary* opts = nullptr;
  if (avcodec_open2(m_context, m_decoder, &opts) < 0) {
    assert(false);
  }
}

Decoder::~Decoder() {
  avcodec_free_context(&m_context);

  m_context = nullptr;
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

  auto bytes = yuv420_to_bgra(y, u, v, height, width, y_pad, uv_pad);

  av_frame_free(&frame);
  return Image {std::move(bytes.data), Size {height, width}};
}

}