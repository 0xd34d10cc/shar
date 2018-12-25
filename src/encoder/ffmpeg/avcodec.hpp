#pragma once

#include <vector>

#include "disable_warnings_push.hpp"
extern "C" {
#include <libavcodec/avcodec.h>
}
#include "disable_warnings_pop.hpp"

#include "size.hpp"
#include "logger.hpp"
#include "config.hpp"
#include "network/packet.hpp"
#include "capture/frame.hpp"
#include "Options.hpp"
#include "avcontext.hpp"


struct AVCodec;
class ContextPtr;

namespace shar::codecs::ffmpeg {

class AVCodecPtr {

public:
  AVCodecPtr();
  AVCodecPtr(Logger& logger, const ConfigPtr& config, ContextPtr& context, Options& options, Size frame_size, std::size_t fps);
  AVCodec* get_ptr() const;
  ~AVCodecPtr();

private:
  AVCodec* m_encoder;
};
}