#include <cassert> // assert
#include <numeric> // std::gcd

#include "disable_warnings_push.hpp"
extern "C" {
#include <libavutil/frame.h>
}
#include "disable_warnings_pop.hpp"

#include "frame.hpp"
#include "encoder/convert.hpp"


namespace shar::encoder::ffmpeg {

Frame::Frame(FramePtr frame, std::unique_ptr<std::uint8_t[]> data)
  : m_frame(std::move(frame))
  , m_data(std::move(data))
  {}

Frame Frame::from_bgra(const char* data, Size size) {
  auto image = bgra_to_yuv420(data, size);
  assert(image.data.size == image.y_size + image.u_size + image.v_size);

  auto frame = FramePtr(av_frame_alloc());
  assert(frame);

  frame->format = AV_PIX_FMT_YUV420P;
  frame->height = static_cast<int>(size.height());
  frame->width = static_cast<int>(size.width());

  frame->data[0] = image.data.data.get();
  frame->data[1] = image.data.data.get() + image.y_size;
  frame->data[2] = image.data.data.get() + image.y_size + image.u_size;

  frame->extended_data = &frame->data[0];

  frame->linesize[0] = static_cast<int>(size.width());
  frame->linesize[1] = static_cast<int>(size.width() / 2);
  frame->linesize[2] = static_cast<int>(size.width() / 2);

  std::size_t divisor = std::gcd(size.width(), size.height());
  frame->sample_aspect_ratio.num = static_cast<int>(size.width() / divisor);
  frame->sample_aspect_ratio.den = static_cast<int>(size.height() / divisor);

  return Frame(std::move(frame), std::move(image.data.data));
}

// all data
std::uint8_t* Frame::data() noexcept {
  return m_data.get();
}

const std::uint8_t* Frame::data() const noexcept {
  return m_data.get();
}

// NOTE: expects no padding
std::size_t Frame::total_size() const noexcept {
  std::size_t w = width();
  std::size_t h = height();
  std::size_t pixels = w * h;
  // 12 bits per pixel
  return pixels + pixels / 2;
}

std::size_t Frame::width() const noexcept {
  return m_frame ? m_frame->width : 0;
}

std::size_t Frame::height() const noexcept {
  return m_frame ? m_frame->height : 0;
}

Frame::Channel Frame::y() const noexcept {
  return m_frame ? Channel{ m_frame->data[0], width(), height(), total_size() }
                 : Channel{};
}

Frame::Channel Frame::u() const noexcept {
  return m_frame ? Channel{ m_frame->data[1], width() / 2, height() / 2, total_size() / 4 }
                 : Channel{};
}

Frame::Channel Frame::v() const noexcept {
  return m_frame ? Channel{ m_frame->data[2], width() / 2, height() / 2, total_size() / 4 }
                 : Channel{};
}

AVFrame* Frame::raw() noexcept {
  return m_frame.get();
}

const AVFrame* Frame::raw() const noexcept {
  return m_frame.get();
}

void Frame::set_timestamp(TimePoint t) noexcept {
  m_time = t;
}

TimePoint Frame::timestamp() const noexcept {
  return m_time;
}

}