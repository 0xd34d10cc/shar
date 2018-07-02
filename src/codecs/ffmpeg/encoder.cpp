#include <cassert>


#include "disable_warnings_push.hpp"
extern "C" {
#include <libavcodec/avcodec.h>
}
#include "disable_warnings_pop.hpp"

#include "codecs/convert.hpp"
#include "codecs/ffmpeg/encoder.hpp"


namespace shar::codecs::ffmpeg {

Encoder::Encoder(Size /*frame_size*/, std::size_t bitrate, std::size_t fps)
    : m_context(nullptr)
    , m_encoder(nullptr) {
  m_encoder = avcodec_find_encoder_by_name("h264_nvenc");
  m_context = avcodec_alloc_context3(m_encoder);

  assert(m_encoder);
  assert(m_context);
  std::fill_n(reinterpret_cast<char*>(m_context), sizeof(AVCodecContext), 0);

  m_context->max_b_frames            = 1;
  m_context->gop_size                = 10;
  m_context->bit_rate                = static_cast<int>(bitrate);
  m_context->time_base.num           = 1;
  m_context->time_base.den           = static_cast<int>(fps);
  m_context->pix_fmt                 = AV_PIX_FMT_YUV420P;
  m_context->width                   = 1920;
  m_context->height                  = 1080;
  m_context->max_pixels              = 1920 * 1080;
  // "unknown"
  m_context->sample_aspect_ratio.num = 16;
  m_context->sample_aspect_ratio.den = 9;

  AVDictionary* opts = nullptr;
  if (av_dict_set(&opts, "b", "2.5M", 0) < 0) {
    assert(false);
  }

  if (avcodec_open2(m_context, m_encoder, &opts) < 0) {
    assert(false);
  }
}


Encoder::~Encoder() {
  avcodec_free_context(&m_context);
  m_context = nullptr;
  // why there is no encoder_free function in libavcodec?
  m_encoder = nullptr;
}

std::vector<Packet> Encoder::encode(const shar::Image& image) {
  auto [y, u, v] = bgra_to_yuv420(image);

  AVFrame* frame = av_frame_alloc();
  std::fill_n(reinterpret_cast<char*>(frame), sizeof(AVFrame), 0);
  frame->format = AV_PIX_FMT_YUV420P;
  frame->height = static_cast<int>(image.height());
  frame->width  = static_cast<int>(image.width());

  frame->data[0] = y.data.get();
  frame->data[1] = u.data.get();
  frame->data[2] = v.data.get();

  frame->linesize[0] = static_cast<int>(image.width());
  frame->linesize[1] = static_cast<int>(image.width() / 2);
  frame->linesize[2] = static_cast<int>(image.width() / 2);

  int ret = avcodec_send_frame(m_context, frame);

  if (ret == AVERROR(EAGAIN) || ret == 0) {
    std::vector<Packet> packets;

    AVPacket packet;
    std::fill_n(reinterpret_cast<char*>(&packet), sizeof(AVPacket), 0);

    ret = avcodec_receive_packet(m_context, &packet);
    while (ret != AVERROR(EAGAIN)) {
      // convert packet

      std::size_t size = static_cast<std::size_t>(packet.size);
      auto        data = std::make_unique<std::uint8_t[]>(size);
      std::copy(packet.data, packet.data + size, data.get());

      packets.emplace_back(std::move(data), size);

      // reset packet
      std::fill_n(reinterpret_cast<char*>(&packet), sizeof(AVPacket), 0);
      ret = avcodec_receive_packet(m_context, &packet);
    }

    return packets;
  }

  assert(false);
  return {};
}

}