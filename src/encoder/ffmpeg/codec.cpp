#include "codec.hpp"

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

  m_encoder = AVCodecPtr(m_logger, config, m_context, opts, frame_size, fps);

  if (opts.count() != 0) {
    m_logger.warning("Following {} options were not found: {}",
      opts.count(), opts.to_string());
  }
}

std::vector<Packet> Codec::encode(const Frame & image)
{
  return std::vector<Packet>();
}

int Codec::get_pts() {
  return static_cast<int>(CLOCK_RATE / static_cast<unsigned int>(m_context.get_ptr()->time_base.den) * m_frame_counter++);
}

Codec::~Codec() {
}
}