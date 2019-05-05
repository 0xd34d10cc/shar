#include <cassert> // assert
#include <numeric> // std::gcd

#include "disable_warnings_push.hpp"
extern "C" {
#include <libavutil/frame.h>
}
#include "disable_warnings_pop.hpp"

#include "frame.hpp"
#include "codec/convert.hpp"


namespace shar::codec::ffmpeg {

Frame::Frame(FramePtr frame, std::unique_ptr<std::uint8_t[]> data)
  : m_frame(std::move(frame))
  , m_data(std::move(data))
  {}

Frame Frame::from_bgra(const char* data, Size size) {
  auto image = bgra_to_yuv420(data, size);
  assert(image.size == image.y_size + image.u_size + image.v_size);

  auto frame = FramePtr(av_frame_alloc());
  assert(frame);

  frame->format = AV_PIX_FMT_YUV420P;
  frame->height = static_cast<int>(size.height());
  frame->width = static_cast<int>(size.width());

  frame->data[0] = image.data.get();
  frame->data[1] = image.data.get() + image.y_size;
  frame->data[2] = image.data.get() + image.y_size + image.u_size;

  frame->extended_data = &frame->data[0];

  frame->linesize[0] = static_cast<int>(size.width());
  frame->linesize[1] = static_cast<int>(size.width() / 2);
  frame->linesize[2] = static_cast<int>(size.width() / 2);

  std::size_t divisor = std::gcd(size.width(), size.height());
  frame->sample_aspect_ratio.num = static_cast<int>(size.width() / divisor);
  frame->sample_aspect_ratio.den = static_cast<int>(size.height() / divisor);

  return Frame(std::move(frame), std::move(image.data));
}

Slice Frame::to_bgra() const {
  AVFrame* frame = m_frame.get();
  if (!frame) {
    return Slice{};
  }

  std::size_t height = static_cast<std::size_t>(frame->height);
  std::size_t width = static_cast<std::size_t>(frame->width);
  std::size_t y_pad = static_cast<std::size_t>(frame->linesize[0]) - width;
  std::size_t uv_pad = static_cast<std::size_t>(frame->linesize[1]) - width / 2;
  uint8_t* y = frame->data[0];
  uint8_t* u = frame->data[1];
  uint8_t* v = frame->data[2];

  return yuv420_to_bgra(y, u, v, height, width, y_pad, uv_pad);
}

Frame Frame::alloc() {
  auto frame = FramePtr(av_frame_alloc());
  assert(frame);
  return Frame(std::move(frame), nullptr);
}

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

Size Frame::sizes() const noexcept {
  return Size{height(), width()};
}

std::size_t Frame::width() const noexcept {
  return m_frame ? static_cast<std::size_t>(m_frame->width) : 0;
}

std::size_t Frame::height() const noexcept {
  return m_frame ? static_cast<std::size_t>(m_frame->height) : 0;
}

static Frame::Channel make_channel(const std::uint8_t* data,
                                   std::size_t width,
                                   std::size_t height) {
  return Frame::Channel{data, width, height, width * height};
}

Frame::Channel Frame::y() const noexcept {
  return m_frame ? make_channel(m_frame->data[0], width(), height()) : Channel{};
}

Frame::Channel Frame::u() const noexcept {
  return m_frame ? make_channel(m_frame->data[1], width() / 2, height() / 2) : Channel{};
}

Frame::Channel Frame::v() const noexcept {
  return m_frame ? make_channel(m_frame->data[2], width() / 2, height() / 2) : Channel{};
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
