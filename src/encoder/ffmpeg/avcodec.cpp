#include "avcodec.hpp"
#include "avcontext.hpp"


namespace shar::codecs::ffmpeg {

AVCodecPtr::AVCodecPtr() : m_encoder(nullptr)
{
}

AVCodecPtr::AVCodecPtr(Logger& logger
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
      m_encoder = codec;
      ContextPtr cont(kbits, codec, frame_size, fps);
      if (avcodec_open2(cont.get_ptr(), codec, &opts.get_ptr()) >= 0) {
        logger.info("Using {} encoder", codec_name);
        context = std::move(cont);
        return;
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
      if (avcodec_open2(cont.get_ptr(), codec, &opts.get_ptr()) >= 0) {
        logger.info("Using {} encoder", name);
        context = std::move(cont);
        m_encoder = codec;
        return;
      }
    }
  }

  logger.warning("None of hardware accelerated codecs available. Using default h264 encoder");
  m_encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
  context = ContextPtr(kbits, m_encoder, frame_size, fps);
  if (avcodec_open2(context.get_ptr(), m_encoder, &opts.get_ptr()) >= 0) {

    logger.info("Using {} encoder", codec_name);
  }
}

AVCodec* AVCodecPtr::get_ptr() const {
  return m_encoder;
}

AVCodecPtr::~AVCodecPtr() {
}
}