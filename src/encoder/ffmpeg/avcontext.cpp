#include <numeric> // std::gcd
#include <cassert> // assert

#include "disable_warnings_push.hpp"
extern "C" {
#include <libavcodec/avcodec.h>
}
#include "disable_warnings_pop.hpp"

#include "avcontext.hpp"


namespace shar::encoder::ffmpeg {

ContextPtr::ContextPtr(std::size_t kbits, AVCodec * codec, Size frame_size, std::size_t fps) {
  m_context = AVContextPtr(avcodec_alloc_context3(codec));
  assert(m_context);

  std::fill_n(reinterpret_cast<char*>(m_context.get()), sizeof(AVCodecContext), 0);
  m_context->bit_rate = static_cast<int>(kbits * 1024);
  m_context->time_base.num = 1;
  m_context->time_base.den = static_cast<int>(fps);
  m_context->pix_fmt = AV_PIX_FMT_YUV420P;
  m_context->width = static_cast<int>(frame_size.width());
  m_context->height = static_cast<int>(frame_size.height());
  m_context->max_pixels = m_context->width * m_context->height;

  std::size_t divisor = std::gcd(frame_size.width(), frame_size.height());
  m_context->sample_aspect_ratio.num = static_cast<int>(frame_size.width() / divisor);
  m_context->sample_aspect_ratio.den = static_cast<int>(frame_size.height() / divisor);
}


AVCodecContext* ContextPtr::get() {
  return m_context.get();
}

}
