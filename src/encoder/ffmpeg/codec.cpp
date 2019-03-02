#include "disable_warnings_push.hpp"
extern "C" {
#include <libavcodec/avcodec.h>
}
#include "disable_warnings_pop.hpp"

#include <memory>
#include <optional>

#include "encoder/convert.hpp"
#include "common/time.hpp"
#include "codec.hpp"

namespace {
  static const int    buf_size = 250;
  static const int    prefix_length = 9;
  static const char   log_prefix[prefix_length + 1] = "[ffmpeg] "; // +1 for /0
  static std::optional<shar::Logger> cb_logger;

  static void avlog_callback(void * ptr, int level, const char * fmt, va_list args) {
    if (cb_logger) {
      char buf[buf_size];
      std::memcpy(buf, log_prefix, prefix_length);  
      vsnprintf(buf + prefix_length, buf_size - prefix_length, fmt, args);
      switch (level) {
      case AV_LOG_TRACE:
        cb_logger->trace(buf);
        break;
      case AV_LOG_DEBUG:
      case AV_LOG_VERBOSE:
        cb_logger->debug(buf);
        break;
      case AV_LOG_INFO:
        cb_logger->info(buf);
        break;
      case AV_LOG_WARNING:
        cb_logger->warning(buf);
        break;
      case AV_LOG_ERROR:
        cb_logger->error(buf);
        break;
      case AV_LOG_FATAL:
      case AV_LOG_PANIC:
        cb_logger->critical(buf);
        break;
      default:
        assert(false);
        break;
      }
    }
  }

  static void setup_logging(const shar::OptionsPtr& config, shar::Logger& logger) {
    cb_logger = logger;

    std::map<shar::LogLevel, int> log_levels = {
        { shar::LogLevel::quite, AV_LOG_QUIET },
        { shar::LogLevel::trace, AV_LOG_TRACE },
        { shar::LogLevel::debug, AV_LOG_DEBUG },
        { shar::LogLevel::info, AV_LOG_INFO },
        { shar::LogLevel::warning, AV_LOG_WARNING },
        { shar::LogLevel::critical, AV_LOG_FATAL },
        { shar::LogLevel::error, AV_LOG_ERROR },
    };

    auto loglvl = shar::string_loglvls[config->encoder_loglvl];
    auto available_lvl = log_levels[loglvl];

    av_log_set_level(available_lvl);
  }
}

namespace shar::encoder::ffmpeg {

// Clock rate (number of ticks in 1 second) for H264 video. (RFC 6184 Section 8.2.1)
static const unsigned int CLOCK_RATE = 90000;

Codec::Codec(Context context, Size frame_size, std::size_t fps)
  : Context(std::move(context))
  , m_frame_counter(0) {

  m_full_delay = metrics::Histogram({ "Codec_full_delay", "Delay of capture & codec", "ms" },
                                     m_registry, { 15.0, 30.0, 60.0 });
  ffmpeg::Options opts{};
  for (const auto& [key, value]: m_config->options) {
    if (!opts.set(key.c_str(), value.c_str())) {
      m_logger.error("Failed to set {} encoder option to {}. Ignoring", key, value);
    }
  }

  m_encoder = select_codec(opts, frame_size, fps);
  assert(m_context.get());
  assert(m_encoder);

  setup_logging(m_config, m_logger);

  // ffmpeg will leave all invalid options inside opts
  if (opts.count() != 0) {
    m_logger.warning("Following {} options were not found: {}",
      opts.count(), opts.to_string());
  }
}

std::vector<Unit> Codec::encode(const shar::Frame& image) {
  assert(m_context.get()->width == image.width());
  assert(m_context.get()->height == image.height());

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
  int pts = next_pts();
  frame->pts = pts;

  int ret = avcodec_send_frame(m_context.get(), frame);
  std::vector<Unit> packets;

  assert(ret==0);
  if (ret == 0) {
    auto unit = Unit::allocate();

    ret = avcodec_receive_packet(m_context.get(), unit.raw());
    while (ret != AVERROR(EAGAIN)) {
      unit.raw()->pts = pts;
      packets.emplace_back(std::move(unit));

      unit = Unit::allocate();
      ret = avcodec_receive_packet(m_context.get(), unit.raw());

      // NOTE: this delay is incorrect, because encoder is able to buffer frames.
      const auto delay = Clock::now() - image.timestamp();
      const auto delay_ms = std::chrono::duration_cast<Milliseconds>(delay);
      m_full_delay.Observe(delay_ms.count());
    }
  }

  av_frame_free(&frame);
  return packets;
}

int Codec::next_pts() {
  const auto fps = m_context.get()->time_base.den;
  const auto ticks_per_frame = CLOCK_RATE / fps;
  const auto pts = ticks_per_frame * m_frame_counter;
  m_frame_counter++;
  return pts;
}

AVCodec* Codec::select_codec(Options& opts,
                             Size frame_size,
                             std::size_t fps)
{
  const std::string codec_name = m_config->codec;
  const std::size_t kbits = m_config->bitrate;

  if (!codec_name.empty()) {
    if (auto* codec = avcodec_find_encoder_by_name(codec_name.c_str())) {

      m_logger.info("Using {} encoder from config", codec_name);
      ffmpeg::ContextPtr context{ kbits, codec, frame_size, fps };
      if (avcodec_open2(context.get(), codec, &opts.get_ptr()) >= 0) {
        m_logger.info("Using {} encoder", codec_name);
        m_context = std::move(context);
        return codec;
      }
    }

    m_logger.warning("Encoder {} requested but not found", codec_name);
  }

  static std::array<const char*, 4> codecs = {
      "h264_nvenc", // https://github.com/0xd34d10cc/shar/issues/89
      //"h264_amf", // https://github.com/0xd34d10cc/shar/issues/54
      "h264_qsv", // https://github.com/0xd34d10cc/shar/issues/92
      //"h264_vaapi", // https://github.com/0xd34d10cc/shar/issues/27
      //"h264_v4l2m2m", // https://github.com/0xd34d10cc/shar/issues/112
      "h264_videotoolbox", // https://github.com/0xd34d10cc/shar/issues/84
      "h264_omx" // didn't tested yet
  };

  for (const char* name : codecs) {
    if (auto* codec = avcodec_find_encoder_by_name(name)) {

      ContextPtr context{ kbits, codec, frame_size, fps };
      if (avcodec_open2(context.get(), codec, &opts.get_ptr()) >= 0) {
        m_logger.info("Using {} encoder", name);
        m_context = std::move(context);
        return codec;
      }
    }
  }

  m_logger.warning("None of hardware accelerated codecs available. Using default h264 encoder");
  auto* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
  m_context = ffmpeg::ContextPtr(kbits, codec, frame_size, fps);

  if (avcodec_open2(m_context.get(), codec, &opts.get_ptr()) >= 0) {
   return codec;
  }

  throw std::runtime_error("Failed to initialize default codec");
}

}