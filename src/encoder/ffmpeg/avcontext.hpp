#pragma once

#include <cstdlib>
#include <memory>

#include "size.hpp"


extern "C" {
struct AVCodecContext;
struct AVCodec;

void avcodec_free_context(AVCodecContext **avctx);
}

namespace shar::encoder::ffmpeg {

// TODO: remove this class
class ContextPtr {
public:
  ContextPtr() = default;
  ContextPtr(std::size_t kbits,
             AVCodec* codec,
             Size frame_size,
             std::size_t fps);
  ContextPtr(const ContextPtr&) = delete;
  ContextPtr(ContextPtr&& context) = default;
  ContextPtr operator=(const ContextPtr& rh) = delete;
  ContextPtr& operator=(ContextPtr&& rh) = default;
  ~ContextPtr() = default;

  AVCodecContext* get();

private:
  struct Deleter {
    void operator()(AVCodecContext* context) {
      avcodec_free_context(&context);
    }
  };

  using AVContextPtr = std::unique_ptr<AVCodecContext, Deleter>;
  AVContextPtr m_context;
};

}