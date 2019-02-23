#include "disable_warnings_push.hpp"
extern "C" {
#include <libavcodec/avcodec.h>
}
#include "disable_warnings_pop.hpp"

#include "encoder/convert.hpp"
#include "common/time.hpp"
#include "codec.hpp"



namespace shar::codecs::ffmpeg {

// Clock rate (number of ticks in 1 second) for H264 video. (RFC 6184 Section 8.2.1)
static const unsigned int CLOCK_RATE = 90000;

Codec::Codec(Context context, Size frame_size, std::size_t fps)
  : Context(std::move(context))
  , m_frame_counter(0)
  {

  Options opts{};
<<<<<<< HEAD
  auto options = m_config->get_subconfig("options");
  for (const auto& iter : *options) {
    const char* key = iter.first.c_str();
    const std::string value = iter.second.get_value<std::string>();
=======
  auto options = config->get_subconfig("options");
  for (const auto& [key, value]: *options) {
    if (!value.is_string()) {
      m_logger.error("Invalid encoder option: {}. Expected string", value.dump());
      continue;
    }
>>>>>>> master

    if (!opts.set(key.c_str(), value.get<std::string>().c_str())) {
      m_logger.error("Failed to set {} encoder option to {}. Ignoring", key, value.dump());
    }
  }
  m_full_delay = metrics::Histogram({ "Codec_full_delay", "Delay of capture & codec", "ms" }, m_registry,
    std::vector<double>{15, 30, 60});

  m_encoder = select_codec(opts, frame_size, fps);
  assert(m_context.get());
  assert(m_encoder);

  if (opts.count() != 0) {
    m_logger.warning("Following {} options were not found: {}",
      opts.count(), opts.to_string());
  }
}

std::vector<Packet> Codec::encode(const shar::Frame& image) {
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
  int pts = get_pts();
  frame->pts = pts;

  int ret = avcodec_send_frame(m_context.get(), frame);
  std::vector<Packet> packets;

  assert(ret==0);
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

      packets.emplace_back(std::move(data), size, static_cast<std::uint32_t>(pts), type);

      // reset packet
      // NOTE: according to docs avcodec_receive_packet should call av_packet_unref
      // before doing anything else, but who trust docs?
      av_packet_unref(&packet);
      ret = avcodec_receive_packet(m_context.get(), &packet);
      m_full_delay.Observe(
        std::chrono::duration_cast<Milliseconds>(Clock::now() - image.timestamp()).count());
    }

    av_packet_unref(&packet);
  }

  av_frame_free(&frame);
  return packets;
}

int Codec::get_pts() {
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

  const std::string codec_name = m_config->get<std::string>("codec", "");
  const std::size_t kbits = m_config->get<std::size_t>("bitrate", 5000);

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