#include "codec.hpp"

#include "codec/convert.hpp"
#include "common/time.hpp"

#include "disable_warnings_push.hpp"
extern "C" {
#include <libavcodec/avcodec.h>
}
#include "disable_warnings_pop.hpp"

#include <numeric>
#include <memory>
#include <optional>
#include <array>


static const int buf_size = 250;
static const int prefix_length = 9;
static const char log_prefix[prefix_length + 1] = "[ffmpeg] "; // +1 for /0
static bool logger_enabled = false;

static void avlog_callback(void* /* ptr */, int level, const char* fmt, va_list args) {
  if (!logger_enabled || level > av_log_get_level()) {
    return;
  }

  char buf[buf_size];
  std::memcpy(buf, log_prefix, prefix_length);
  int n = std::vsnprintf(buf + prefix_length, buf_size - prefix_length, fmt, args);

  if (buf[prefix_length + n - 1] == '\n') {
    buf[prefix_length + n - 1] = '\0';
  }

  switch (level) {
    case AV_LOG_TRACE:
      LOG_TRACE(buf);
      break;
    case AV_LOG_DEBUG:
    case AV_LOG_VERBOSE:
      LOG_DEBUG(buf);
      break;
    case AV_LOG_INFO:
      LOG_INFO(buf);
      break;
    case AV_LOG_WARNING:
      LOG_WARN(buf);
      break;
    case AV_LOG_ERROR:
      LOG_ERROR(buf);
      break;
    case AV_LOG_FATAL:
    case AV_LOG_PANIC:
      LOG_FATAL(buf);
      break;
    default:
      assert(false);
      break;
  }
}

static void setup_logging(const shar::ConfigPtr& config) {
  using shar::LogLevel;

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

  logger_enabled = true;
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

  if (logger_enabled) {
    LOG_ERROR("YUV420 pixel format is not supported for this decoder");
  }

  return AV_PIX_FMT_NONE;
}

namespace shar::codec::ffmpeg {

// Clock rate (number of ticks in 1 second) for H264 video. (RFC 6184 Section 8.2.1)
static const int CLOCK_RATE = 90000;

Codec::Codec(Context context, Size frame_size, usize fps)
  : Context(std::move(context))
  , m_frame_counter(0) {

  if (!logger_enabled) {
    setup_logging(m_config);
  }

  open(frame_size, fps);
}

void Codec::open(Size frame_size, usize fps) {
  ffmpeg::Options opts{};
  for (const auto& [key, value] : m_config->options) {
    if (!opts.set(key.c_str(), value.c_str())) {
      LOG_ERROR("Failed to set {} codec option to {}. Ignoring", key, value);
    }
  }

  m_context.reset();
  m_codec = select_codec(opts, frame_size, fps);
  assert(m_context.get());
  assert(m_codec);

  // ffmpeg will leave all invalid options inside opts
  if (opts.count() != 0) {
    LOG_WARN("Following {} options were not found: {}", opts.count(), opts.to_string());
  }
}

std::vector<Unit> Codec::encode(Frame image) {
  auto* context = m_context.get();

  const bool width_changed = static_cast<usize>(context->width) != image.width();
  const bool height_changed = static_cast<usize>(context->height) != image.height();
  const bool resized = width_changed || height_changed;

  if (resized) {
    LOG_WARN(
        "The stream resolution have been changed from {}x{} to {}x{}",
        context->width,
        context->height,
        image.width(),
        image.height()
    );

    usize fps = static_cast<usize>(context->time_base.den);
    open(image.sizes(), fps);
  }

  int pts = next_pts();
  image.raw()->pts = pts;

  int ret = avcodec_send_frame(context, image.raw());
  std::vector<Unit> packets;

  if (ret != 0) {
    report_error(ret);
    return packets;
  }

  auto unit = Unit::allocate();
  ret = avcodec_receive_packet(context, unit.raw());
  while (ret != AVERROR(EAGAIN)) {
    unit.raw()->pts = pts;
    packets.emplace_back(std::move(unit));

    unit = Unit::allocate();
    ret = avcodec_receive_packet(context, unit.raw());

    // NOTE: this delay is incorrect, because encoder is able to buffer frames.
    // TODO: Report codec delay
    // const auto delay = Clock::now() - image.timestamp();
    // const auto delay_ms = std::chrono::duration_cast<Milliseconds>(delay);
    // m_full_delay.Observe(static_cast<double>(delay_ms.count()));
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

  if (ret != 0) {
    report_error(ret);
    return std::nullopt;
  }

  assert(frame.raw()->format == AV_PIX_FMT_YUV420P);
  return std::move(frame);
}

int Codec::next_pts() {
  const int fps = m_context.get()->time_base.den;
  const int ticks_per_frame = CLOCK_RATE / fps;
  const int pts = ticks_per_frame * static_cast<int>(m_frame_counter);
  m_frame_counter++;
  return pts;
}

Codec::AVContextPtr Codec::create_context(usize kbits, AVCodec* codec, shar::Size frame_size, usize fps) {
  auto context = AVContextPtr(avcodec_alloc_context3(codec));
  assert(context);

  context->bit_rate = static_cast<int>(kbits * 1024);
  context->time_base.num = 1;
  context->time_base.den = static_cast<int>(fps);
  context->gop_size = static_cast<int>(fps);
  context->pix_fmt = AV_PIX_FMT_YUV420P;
  context->width = static_cast<int>(frame_size.width());
  context->height = static_cast<int>(frame_size.height());
  // Support resolution up to 4k.
  // NOTE: the values above could be not accurate, since we didn't received any frames yet
  context->max_pixels = 4096 * 2160;
  context->get_buffer2 = avcodec_default_get_buffer2;
  context->get_format = get_format;

  usize divisor = std::gcd(frame_size.width(), frame_size.height());
  context->sample_aspect_ratio.num = static_cast<int>(frame_size.width() / divisor);
  context->sample_aspect_ratio.den = static_cast<int>(frame_size.height() / divisor);

  return context;
}

AVCodec* Codec::select_codec(ffmpeg::Options& opts,
                             Size frame_size,
                             usize fps)
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
  const usize kbits = m_config->bitrate;

  if (!codec_name.empty()) {
    if (auto* codec = find_codec_by_name(codec_name.c_str())) {

      LOG_INFO("Using {} codec from config", codec_name);
      auto context = create_context(kbits, codec, frame_size, fps);
      if (avcodec_open2(context.get(), codec, &opts.get_ptr()) >= 0) {
        LOG_INFO("Using {} codec", codec_name);
        m_context = std::move(context);
        return codec;
      }
    }

    LOG_WARN("Codec {} requested but not found", codec_name);
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
        LOG_INFO("Using {} codec", name);
        m_context = std::move(context);
        return codec;
      }
    }
  }

  LOG_WARN("None of hardware accelerated codecs available. Using default h264 codec");
  auto* codec = find_codec_by_id(AV_CODEC_ID_H264);
  m_context = create_context(kbits, codec, frame_size, fps);

  if (avcodec_open2(m_context.get(), codec, &opts.get_ptr()) >= 0) {
   return codec;
  }

  throw std::runtime_error("Failed to initialize default codec");
}

void Codec::report_error(int code) {
  if (code == 0)
    // nothing to report
    return;

  char message[1024];
  av_strerror(code, message, sizeof(message));
  LOG_ERROR("Codec failure: {}", message);
}

}
