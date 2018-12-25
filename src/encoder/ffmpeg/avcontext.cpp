#include "avcontext.hpp"

namespace shar::codecs::ffmpeg {
ContextPtr::ContextPtr() : m_context(nullptr)
{
}

ContextPtr::ContextPtr(const size_t kbits, AVCodec * codec, Size frame_size, std::size_t fps) {
  m_context = avcodec_alloc_context3(codec);
  assert(m_context);

  std::fill_n(reinterpret_cast<char*>(m_context), sizeof(AVCodecContext), 0);
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

ContextPtr::ContextPtr(ContextPtr && context) : m_context(context.m_context) {
  context.m_context = nullptr;

}

ContextPtr& ContextPtr::operator=(ContextPtr && rh)
{
  if (this == &rh) {
    return *this;
  }
  m_context = rh.m_context;
  rh.m_context = nullptr;
  return *this;
}

AVCodecContext * ContextPtr::get_ptr()
{
  return m_context;
}

ContextPtr::~ContextPtr()
{
  avcodec_free_context(&m_context);
  avcodec_close(m_context);

}
}
