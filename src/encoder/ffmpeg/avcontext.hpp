#pragma once

#include "disable_warnings_push.hpp"
extern "C" {
#include <libavcodec/avcodec.h>
}
#include "disable_warnings_pop.hpp"

#include "size.hpp"
#include "logger.hpp"
#include "config.hpp"
#include "avcodec.hpp"

struct AVCodecContext;

namespace shar::codecs::ffmpeg {
class ContextPtr {
public:
  ContextPtr();
  ContextPtr(const size_t kbits
    , AVCodec* codec
    , Size frame_size
    , std::size_t fps);
  ContextPtr(const ContextPtr&) = delete;
  ContextPtr(ContextPtr&& context);
  ContextPtr operator=(const ContextPtr& rh) = delete;
  ContextPtr& operator=(ContextPtr&& rh);

  AVCodecContext* get_ptr();
  ~ContextPtr();
private:
  AVCodecContext* m_context;
};

}