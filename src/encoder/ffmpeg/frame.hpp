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

  std::uint8_t* data() noexcept;
  const std::uint8_t* data() const noexcept;
  std::size_t total_size() const noexcept;

  std::size_t width() const noexcept;
  std::size_t height() const noexcept;

  struct Channel {
    const std::uint8_t* data{ nullptr };

    std::size_t width{ 0 };
    std::size_t height{ 0 };
    std::size_t size { 0 };
  };

  Channel y() const noexcept;
  Channel u() const noexcept;
  Channel v() const noexcept;

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