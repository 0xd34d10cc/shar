#include <cassert>
#include <cstdlib>
#include <vector>

#include "disable_warnings_push.hpp"
extern "C" {
#include <libavcodec/avcodec.h>
}
#include "disable_warnings_pop.hpp"

#include "codecs/convert.hpp"
#include "codecs/ffmpeg/encoder.hpp"


namespace {

class Options {
public:
  Options()
      : m_opts(nullptr)
  {}

  ~Options() {
    av_dict_free(&m_opts);
  }

  std::size_t count() {
    return static_cast<std::size_t>(av_dict_count(m_opts));
  }

  bool set(const char* key, const char* value) {
    return av_dict_set(&m_opts, key, value, 0 /* flags */) >= 0;
  }

  AVDictionary*& get_ptr() {
    return m_opts;
  }

  std::string to_string() {
    char* buffer = nullptr;
    if (av_dict_get_string(m_opts, &buffer, ':', '|') < 0) {
      std::free(buffer);
      return {};
    }

    std::string opts{buffer};
    return opts;
  }

private:
  AVDictionary* m_opts;
};

}

static int get_pts()
{
  static int static_pts = 0;
  return static_pts++;
}

namespace shar::codecs::ffmpeg {

static AVCodec* select_codec(Logger& logger) {
  static std::array<const char*, 5> codecs = {
      "h264_nvenc",
      "h264_amf",
      //"h264_vaapi", // not supported yet
      "h264_qsv",
      //"h264_v4l2m2m", // not supported yet
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
    , m_encoder(nullptr)
    , m_logger(std::move(logger)) {

  // TODO: allow manual codec selection
  m_encoder = select_codec(m_logger);
  m_context = avcodec_alloc_context3(m_encoder);

  assert(m_encoder);
  assert(m_context);
  std::fill_n(reinterpret_cast<char*>(m_context), sizeof(AVCodecContext), 0);
  const std::string kbits = config.get<std::string>("bitrate", "5000");

  const std::size_t bit_rate = std::stoul(kbits) * 1024;

  m_logger.info("bit rate: {} kbit/s", kbits);
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

  Options opts{};
  for (const auto& iter: config) {
    // TODO: handle errors here
    const char* key = iter.first.c_str();
    const std::string value = iter.second.get_value<std::string>();

    if (!opts.set(key, value.c_str())) {
      m_logger.error("Failed to set {} encoder option to {}. Ignoring", key, value);
    }
  }

  if (avcodec_open2(m_context, m_encoder, &opts.get_ptr()) < 0) {
    assert(false);
  }

  if (opts.count() != 0) {
    m_logger.warning("Following {} options were not found: {}",
                     opts.count(), opts.to_string());
  }
}


Encoder::~Encoder() {
  avcodec_close(m_context);
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
  frame->pts = get_pts();

  int ret = avcodec_send_frame(m_context, frame);
  std::vector<Packet> packets;

  assert(ret == 0);
  if (ret == 0) {

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
      // NOTE: according to docs avcodec_receive_packet should call av_packet_unref
      // before doing anything else, but who trust docs?
      // TODO: check it
      av_packet_unref(&packet);
      ret = avcodec_receive_packet(m_context, &packet);
    }

    av_packet_unref(&packet);
  }

  av_frame_free(&frame);
  return packets;
}

}