#include "disable_warnings_push.hpp"
extern "C" {
#include <libavcodec/avcodec.h>
}
#include "disable_warnings_pop.hpp"

#include <memory>
#include <optional>

#include "codec/convert.hpp"
#include "common/time.hpp"
#include "codec.hpp"


static const int    buf_size = 250;
static const int    prefix_length = 9;
static const char   log_prefix[prefix_length + 1] = "[ffmpeg] "; // +1 for /0
static std::optional<shar::Logger> cb_logger;

static void avlog_callback(void * /* ptr */, int level, const char * fmt, va_list args) {
  if (level > av_log_get_level()) {
    return;
  }

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
  using shar::LogLevel;

  cb_logger = logger;

  const auto log_level_to_ffmpeg = [](LogLevel level) {
    switch (level) {
      case LogLevel::Trace:
        return AV_LOG_TRACE;
      case LogLevel::Debug:
        return AV_LOG_DEBUG;
      case LogLevel::Info:
        return AV_LOG_INFO;
      case LogLevel::Warning:
        return AV_LOG_WARNING;
      case LogLevel::Critical:
        return AV_LOG_FATAL;
      case LogLevel::Error:
        return AV_LOG_ERROR;
      case LogLevel::None:
        return AV_LOG_QUIET;
      default:
        throw std::runtime_error("Unknown log level");
    }
  };

  av_log_set_level(log_level_to_ffmpeg(config->encoder_log_level));
  av_log_set_callback(avlog_callback);
}

static AVPixelFormat get_format(AVCodecContext* /*ctx*/, const enum AVPixelFormat* pix_fmts) {
  const enum AVPixelFormat* p;
  for (p = pix_fmts; *p != -1; p++) {
    if (*p == AV_PIX_FMT_YUV420P) {
      return *p;
    }
  }

  if (cb_logger) {
    cb_logger->error("YUV420 pixel format is not supported for this decoder");
  }

  return AV_PIX_FMT_NONE;
}

namespace shar::codec::ffmpeg {

// Clock rate (number of ticks in 1 second) for H264 video. (RFC 6184 Section 8.2.1)
static const int CLOCK_RATE = 90000;

Codec::Codec(Context context, Size frame_size, std::size_t fps)
  : Context(std::move(context))
  , m_frame_counter(0) {

  if (!cb_logger) {
    setup_logging(m_config, m_logger);
  }

  m_full_delay = metrics::Histogram({ "Codec_full_delay", "Delay of capture & codec", "ms" },
                                     m_registry, { 5.0, 10.0, 15.0, 30.0 });
  ffmpeg::Options opts{};
  for (const auto& [key, value]: m_config->options) {
    if (!opts.set(key.c_str(), value.c_str())) {
      m_logger.error("Failed to set {} codec option to {}. Ignoring", key, value);
    }
  }

  m_codec = select_codec(opts, frame_size, fps);
  assert(m_context.get());
  assert(m_codec);


  // ffmpeg will leave all invalid options inside opts
  if (opts.count() != 0) {
    m_logger.warning("Following {} options were not found: {}",
      opts.count(), opts.to_string());
  }
}

std::vector<Unit> Codec::encode(Frame image) {
  auto* context = m_context.get();
  assert(static_cast<std::size_t>(context->width) == image.width());
  assert(static_cast<std::size_t>(context->height) == image.height());

  int pts = next_pts();
  image.raw()->pts = pts;

  int ret = avcodec_send_frame(context, image.raw());
  std::vector<Unit> packets;

  assert(ret==0);
  if (ret == 0) {
    auto unit = Unit::allocate();

    ret = avcodec_receive_packet(context, unit.raw());
    while (ret != AVERROR(EAGAIN)) {
      unit.raw()->pts = pts;
      packets.emplace_back(std::move(unit));

      unit = Unit::allocate();
      ret = avcodec_receive_packet(context, unit.raw());

      // NOTE: this delay is incorrect, because encoder is able to buffer frames.
      const auto delay = Clock::now() - image.timestamp();
      const auto delay_ms = std::chrono::duration_cast<Milliseconds>(delay);
      m_full_delay.Observe(static_cast<double>(delay_ms.count()));
    }
  }

  return packets;
}

std::optional<Frame> Codec::decode(Unit unit) {
  int ret = avcodec_send_packet(m_context.get(), unit.raw());
  if (ret != 0) {
    return std::nullopt;
  }

  assert(ret == 0);

  auto frame = Frame::alloc();
  ret = avcodec_receive_frame(m_context.get(), frame.raw());

  // not enough data to receive
  if (ret == AVERROR(EAGAIN)) {
    return std::nullopt;
  }

  // error
  assert(ret == 0);
  assert(frame.raw()->format == AV_PIX_FMT_YUV420P);
  return frame;
}

int Codec::next_pts() {
  const int fps = m_context.get()->time_base.den;
  const int ticks_per_frame = CLOCK_RATE / fps;
  const int pts = ticks_per_frame * static_cast<int>(m_frame_counter);
  m_frame_counter++;
  return pts;
}

Codec::AVContextPtr Codec::create_context(std::size_t kbits, AVCodec* codec, shar::Size frame_size, std::size_t fps) {
  auto context = AVContextPtr(avcodec_alloc_context3(codec));
  assert(context);

  context->bit_rate = static_cast<int>(kbits * 1024);
  context->time_base.num = 1;
  context->time_base.den = static_cast<int>(fps);
  context->gop_size = static_cast<int>(fps);
  context->pix_fmt = AV_PIX_FMT_YUV420P;
  context->width = static_cast<int>(frame_size.width());
  context->height = static_cast<int>(frame_size.height());
  context->max_pixels = context->width * context->height;
  context->get_buffer2 = avcodec_default_get_buffer2;
  context->get_format = get_format;

  std::size_t divisor = std::gcd(frame_size.width(), frame_size.height());
  context->sample_aspect_ratio.num = static_cast<int>(frame_size.width() / divisor);
  context->sample_aspect_ratio.den = static_cast<int>(frame_size.height() / divisor);

  return context;
}

AVCodec* Codec::select_codec(ffmpeg::Options& opts,
                             Size frame_size,
                             std::size_t fps)
{
  const auto find_codec_by_name = [this](const char* name) {
    if (m_config->connect) {
      return avcodec_find_decoder_by_name(name);
    }
    else {
      return avcodec_find_encoder_by_name(name);
    }
  };

  const auto find_codec_by_id = [this](AVCodecID id) {
    if (m_config->connect) {
      return avcodec_find_decoder(id);
    }
    else {
      return avcodec_find_encoder(id);
    }
  };

  const std::string codec_name = m_config->codec;
  const std::size_t kbits = m_config->bitrate;

  if (!codec_name.empty()) {
    if (auto* codec = find_codec_by_name(codec_name.c_str())) {

      m_logger.info("Using {} codec from config", codec_name);
      auto context = create_context(kbits, codec, frame_size, fps);
      if (avcodec_open2(context.get(), codec, &opts.get_ptr()) >= 0) {
        m_logger.info("Using {} codec", codec_name);
        m_context = std::move(context);
        return codec;
      }
    }

    m_logger.warning("Codec {} requested but not found", codec_name);
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
    if (auto* codec = find_codec_by_name(name)) {
      auto context = create_context(kbits, codec, frame_size, fps);

      if (avcodec_open2(context.get(), codec, &opts.get_ptr()) >= 0) {
        m_logger.info("Using {} codec", name);
        m_context = std::move(context);
        return codec;
      }
    }
  }

  m_logger.warning("None of hardware accelerated codecs available. Using default h264 codec");
  auto* codec = find_codec_by_id(AV_CODEC_ID_H264);
  m_context = create_context(kbits, codec, frame_size, fps);

  if (avcodec_open2(m_context.get(), codec, &opts.get_ptr()) >= 0) {
   return codec;
  }

  throw std::runtime_error("Failed to initialize default codec");
}

}
