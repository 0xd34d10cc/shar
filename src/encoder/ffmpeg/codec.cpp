#include "disable_warnings_push.hpp"
extern "C" {
#include <libavcodec/avcodec.h>
}
#include "disable_warnings_pop.hpp"

#include "codec.hpp"
#include "../convert.hpp"


namespace shar::codecs::ffmpeg {

static const unsigned int CLOCK_RATE = 90000;

Codec::Codec(Size frame_size, std::size_t fps, Logger logger, const ConfigPtr& config)
  : m_logger(std::move(logger))
  , m_frame_counter(0) {

  Options opts{};
  auto options = config->get_subconfig("options");
  for (const auto& iter : *options) {
    const char* key = iter.first.c_str();
    const std::string value = iter.second.get_value<std::string>();

    if (!opts.set(key, value.c_str())) {
      m_logger.error("Failed to set {} encoder option to {}. Ignoring", key, value);
    }
  }

  m_encoder = select_codec(m_logger, config, m_context, opts, frame_size, fps);
  assert(m_context.get());
  assert(m_encoder);

  if (opts.count() != 0) {
    m_logger.warning("Following {} options were not found: {}",
      opts.count(), opts.to_string());
  }
}

std::vector<Packet> Codec::encode(const shar::Frame& image) {
  auto[y, u, v] = bgra_to_yuv420(image);

  AVFrame* frame = av_frame_alloc();
  std::fill_n(reinterpret_cast<char*>(frame), sizeof(AVFrame), 0);
  frame->format = AV_PIX_FMT_YUV420P;
  frame->height = static_cast<int>(image.height());
  frame->width = static_cast<int>(image.width());

  frame->data[0] = y.data.get();
  frame->data[1] = u.data.get();
  frame->data[2] = v.data.get();

  frame->linesize[0] = static_cast<int>(image.width());
  frame->linesize[1] = static_cast<int>(image.width() / 2);
  frame->linesize[2] = static_cast<int>(image.width() / 2);
  frame->pts = get_pts();

  int ret = avcodec_send_frame(m_context.get(), frame);
  std::vector<Packet> packets;
  bool flag = ret == 0;
  assert(flag);

  if (ret == 0) {

    AVPacket packet;
    std::fill_n(reinterpret_cast<char*>(&packet), sizeof(AVPacket), 0);

    ret = avcodec_receive_packet(m_context.get(), &packet);
    while (ret != AVERROR(EAGAIN)) {
      auto size = static_cast<std::size_t>(packet.size);
      auto data = std::make_unique<std::uint8_t[]>(size);
      std::copy(packet.data, packet.data + size, data.get());

      const bool is_IDR = (packet.flags & AV_PKT_FLAG_KEY) != 0;
      const auto type = is_IDR ? Packet::Type::IDR : Packet::Type::Unknown;

      packets.emplace_back(std::move(data), size, type);

      // reset packet
      // NOTE: according to docs avcodec_receive_packet should call av_packet_unref
      // before doing anything else, but who trust docs?
      av_packet_unref(&packet);
      ret = avcodec_receive_packet(m_context.get(), &packet);
    }

    av_packet_unref(&packet);
  }

  av_frame_free(&frame);
  return packets;
}

int Codec::get_pts() {
  return static_cast<int>(CLOCK_RATE / static_cast<unsigned int>(m_context.get()->time_base.den) * m_frame_counter++);
}

AVCodec * Codec::select_codec(Logger& logger
  , const ConfigPtr& config
  , ContextPtr& context
  , Options& opts
  , Size frame_size
  , std::size_t fps) {

  const std::string codec_name = config->get<std::string>("codec", "");
  const std::size_t kbits = config->get<std::size_t>("bitrate", 5000);

  if (!codec_name.empty()) {
    if (auto* codec = avcodec_find_encoder_by_name(codec_name.c_str())) {

      logger.info("Using {} encoder from config", codec_name);
      ContextPtr cont(kbits, codec, frame_size, fps);
      if (avcodec_open2(cont.get(), codec, &opts.get_ptr()) >= 0) {
        logger.info("Using {} encoder", codec_name);
        context = std::move(cont);
        return codec;
      }
    }

    logger.warning("Encoder {} requested but not found", codec_name);
  }

  static std::array<const char*, 5> codecs = {
      "h264_nvenc",
      "h264_amf",
      "h264_qsv",
      // TODO: implement
      //"h264_vaapi",
      //"h264_v4l2m2m",
      "h264_videotoolbox",
      "h264_omx"
  };

  for (const char* name : codecs) {
    if (auto* codec = avcodec_find_encoder_by_name(name)) {

      ContextPtr cont(kbits, codec, frame_size, fps);
      if (avcodec_open2(cont.get(), codec, &opts.get_ptr()) >= 0) {
        logger.info("Using {} encoder", name);
        context = std::move(cont);
        return codec;
      }
    }
  }

  logger.warning("None of hardware accelerated codecs available. Using default h264 encoder");
  auto* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
  context = ContextPtr(kbits, codec, frame_size, fps);
  return codec;
}

}