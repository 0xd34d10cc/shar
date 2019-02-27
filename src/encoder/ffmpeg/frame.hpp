#pragma once

#include <cstdint> // std::uint8_t
#include <cstdlib> // std::size_t
#include <memory>  // std::unique_ptr
#include <tuple>   // std::tuple

#include "time.hpp"
#include "size.hpp"


extern "C" {
struct AVFrame;
void av_frame_free(AVFrame** frame);
}

namespace shar::encoder::ffmpeg {

template <typename T>
using Triple = std::tuple<T, T, T>;

class Frame {
public:
  Frame() = default;
  Frame(const Frame&) = delete;
  Frame(Frame&&) noexcept = default;
  Frame& operator=(const Frame&) = delete;
  Frame& operator=(Frame&&) noexcept = default;
  ~Frame() = default;

  static Frame from_bgra(const char* data, Size size);

  std::uint8_t* channel_data(std::size_t index) noexcept;
  const std::uint8_t* channel_data(std::size_t index) const noexcept;
  std::size_t channel_size(std::size_t index) const noexcept;

  Triple<std::size_t> sizes() const noexcept;

  std::size_t width() const noexcept;
  std::size_t height() const noexcept;

  std::uint8_t* ys() noexcept;
  const std::uint8_t* ys() const noexcept;
  std::size_t y_len() const noexcept;

  std::uint8_t* us() noexcept;
  const std::uint8_t* us() const noexcept;
  std::size_t u_len() const noexcept;

  std::uint8_t* vs() noexcept;
  const std::uint8_t* vs() const noexcept;
  std::size_t v_len() const noexcept;

  std::uint8_t* data() noexcept;
  const std::uint8_t* data() const noexcept;
  std::size_t total_size() const noexcept;

  AVFrame* raw() noexcept;
  const AVFrame* raw() const noexcept;

  void set_timestamp(TimePoint t) noexcept;
  TimePoint timestamp() const noexcept;

private:
  struct Deleter {
    void operator()(AVFrame* frame) {
      av_frame_free(&frame);
    }
  };

  using FramePtr = std::unique_ptr<AVFrame, Deleter>;
  Frame(FramePtr frame, std::unique_ptr<std::uint8_t[]> data);

  FramePtr m_frame;
  std::unique_ptr<std::uint8_t[]> m_data;
  TimePoint m_time;
};

}