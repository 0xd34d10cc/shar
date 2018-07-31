#include <cassert>

#include "disable_warnings_push.hpp"
extern "C" {
#include <libavcodec/avcodec.h>
}
#include "disable_warnings_pop.hpp"

#include "codecs/convert.hpp"
#include "codecs/ffmpeg/encoder.hpp"


namespace shar::codecs::ffmpeg {

static AVCodec* select_codec(Logger& logger) {
  static std::array<const char*, 5> codecs = {
      "h264_nvenc",
      "h264_amf",
      //"h264_vaapi", not supported yet
      "h264_qsv",
      //"h264_v4l2m2m", not supported yet
      "h264_videotoolbox",
      "h264_omx"
  };

  for (const char* name: codecs) {
    if (auto* codec = avcodec_find_encoder_by_name(name)) {
      logger.info("Using {} encoder", name);
      return codec;
    }
  }
  logger.info("Using default h264 encoder");
  return avcodec_find_encoder(AV_CODEC_ID_H264);
}


Encoder::Encoder(Size frame_size, std::size_t fps, Logger logger, const Config& config)
    : m_context(nullptr)
    , m_logger(std::move(logger))
    , m_encoder(nullptr) {

  // TODO: allow manual codec selection
  m_logger.info("Encoder initialization started...");
  m_encoder = select_codec(logger);
  m_logger.info("Encoder initialization started11...");
  m_context = avcodec_alloc_context3(m_encoder);

  assert(m_encoder);
  assert(m_context);
  std::fill_n(reinterpret_cast<char*>(m_context), sizeof(AVCodecContext), 0);
  std::string kbits;
  m_logger.info("Encoder initialization started11...");
  try {
  kbits = config.get<std::string>("bitrate", "5000");
  } catch (boost::exception const&  ex) {
    m_logger.info("Encoder fucked up on bitrate parse");
  }
    
  std::size_t bit_rate = std::stoul(kbits) * 1024;
  m_context->bit_rate                = static_cast<int>(bit_rate);
  m_context->time_base.num           = 1;
  m_context->time_base.den           = static_cast<int>(fps);
  m_context->pix_fmt                 = AV_PIX_FMT_YUV420P;
  m_context->width                   = static_cast<int>(frame_size.width());
  m_context->height                  = static_cast<int>(frame_size.height());
  m_context->max_pixels              = m_context->width * m_context->height;
  // FIXME: unhardcode
  m_context->sample_aspect_ratio.num = 16;
  m_context->sample_aspect_ratio.den = 9;

  AVDictionary* opts = nullptr;
  for (const auto& iter: config) {
    // TODO: handle errors here
    av_dict_set(&opts, iter.first.c_str(), iter.second.get_value<std::string>().c_str(), 0 /* flags */);
  }

  if (avcodec_open2(m_context, m_encoder, &opts) < 0) {
    assert(false);
  }
}


Encoder::~Encoder() {
  avcodec_free_context(&m_context);

  m_context = nullptr;
  m_encoder = nullptr;
}

std::vector<Packet> Encoder::encode(const shar::Image& image) {
  auto[y, u, v] = bgra_to_yuv420(image);

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

  if (ret == 0) {
    std::vector<Packet> packets;

    AVPacket packet;
    std::fill_n(reinterpret_cast<char*>(&packet), sizeof(AVPacket), 0);

    ret = avcodec_receive_packet(m_context, &packet);
    while (ret != AVERROR(EAGAIN)) {
      std::size_t size = static_cast<std::size_t>(packet.size);
      auto        data = std::make_unique<std::uint8_t[]>(size);
      std::copy(packet.data, packet.data + size, data.get());

      const bool is_IDR = (packet.flags & AV_PKT_FLAG_KEY) != 0;
      const auto type   = is_IDR ? Packet::Type::IDR : Packet::Type::Unknown;

      packets.emplace_back(std::move(data), size, type);

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