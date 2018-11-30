#include <cassert>
#include <cstdlib>
#include <vector>
#include <numeric>

#include "disable_warnings_push.hpp"
extern "C" {
#include <libavcodec/avcodec.h>
}
#include "disable_warnings_pop.hpp"

#include "convert.hpp"
#include "ffmpeg.hpp"


namespace {

// Clock rate (number of ticks in 1 second) for H264 video. (RFC 6184 Section 8.2.1)
static const unsigned int CLOCK_RATE = 90000;

class Options {
public:
  Options() = default;
  Options(const Options&) = delete;
  Options& operator=(const Options&) = delete;

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
    if (av_dict_get_string(m_opts, &buffer, '=', ',') < 0) {
      std::free(buffer);
      return {};
    }

    std::string opts{buffer};
    return opts;
  }

private:
  AVDictionary* m_opts = nullptr;
};

}

namespace shar::codecs::ffmpeg {

static AVCodecContext* make_context(const ConfigPtr& config, AVCodec* codec, Size frame_size, std::size_t fps) {

  AVCodecContext* context = avcodec_alloc_context3(codec);
  assert(context);

  std::fill_n(reinterpret_cast<char*>(context), sizeof(AVCodecContext), 0);
  const std::size_t kbits = config->get<std::size_t>("bitrate", 5000);
  context->bit_rate = static_cast<int>(kbits * 1024);
  context->time_base.num = 1;
  context->time_base.den = static_cast<int>(fps);
  context->pix_fmt = AV_PIX_FMT_YUV420P;
  context->width = static_cast<int>(frame_size.width());
  context->height = static_cast<int>(frame_size.height());
  context->max_pixels = context->width * context->height;

  std::size_t divisor = std::gcd(frame_size.width(), frame_size.height());
  context->sample_aspect_ratio.num = static_cast<int>(frame_size.width() / divisor);
  context->sample_aspect_ratio.den = static_cast<int>(frame_size.height() / divisor);

  return context;
}

static AVCodec* select_codec(Logger& logger,const ConfigPtr& config, Options* opts, Size frame_size, std::size_t fps){

  const std::string codec_name = config->get<std::string>("codec", "");
  if (!codec_name.empty()) {
    if (auto* codec = avcodec_find_encoder_by_name(codec_name.c_str())) {
      logger.info("Using {} encoder from config", codec_name);
      return codec;
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

  for (const char* name : codecs){
    if (auto* codec = avcodec_find_encoder_by_name(name)) {

      auto* context = make_context(config, codec, frame_size, fps);
      if (avcodec_open2(context, codec, &opts->get_ptr()) >= 0) {
        logger.info("Using {} encoder", name);
        return codec;
      }
      else {
        avcodec_close(context);
      }
    }
  }

  logger.warning("None of hardware accelerated codecs available. Using default h264 encoder");
  return avcodec_find_encoder(AV_CODEC_ID_H264);
}

Codec::Codec(Size frame_size, std::size_t fps, Logger logger, const ConfigPtr& config)
    : m_context(nullptr)
    , m_encoder(nullptr)
    , m_logger(std::move(logger))
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

  m_encoder = select_codec(m_logger, config, &opts, frame_size, fps);
  assert(m_encoder);

  m_context = make_context(config, m_encoder, frame_size, fps);
  if (avcodec_open2(m_context, m_encoder, &opts.get_ptr())<0) {
    assert(false);
  }

  if (opts.count() != 0) {
    m_logger.warning("Following {} options were not found: {}",
                     opts.count(), opts.to_string());
  }
}


Codec::~Codec() {
  avcodec_close(m_context);
  avcodec_free_context(&m_context);

  m_context = nullptr;
  m_encoder = nullptr;
}

std::vector<Packet> Codec::encode(const shar::Frame& image) {
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
      auto size = static_cast<std::size_t>(packet.size);
      auto data = std::make_unique<std::uint8_t[]>(size);
      std::copy(packet.data, packet.data + size, data.get());

      const bool is_IDR = (packet.flags & AV_PKT_FLAG_KEY) != 0;
      const auto type   = is_IDR ? Packet::Type::IDR : Packet::Type::Unknown;

      packets.emplace_back(std::move(data), size, type);

      // reset packet
      // NOTE: according to docs avcodec_receive_packet should call av_packet_unref
      // before doing anything else, but who trust docs?
      av_packet_unref(&packet);
      ret = avcodec_receive_packet(m_context, &packet);
    }

    av_packet_unref(&packet);
  }

  av_frame_free(&frame);
  return packets;
}

// PTS calculation formula: (1 / FPS) * sample rate * frame number
int Codec::get_pts() {
  return static_cast<int>(CLOCK_RATE / static_cast<unsigned int>(m_context->time_base.den) * m_frame_counter++);
}

}