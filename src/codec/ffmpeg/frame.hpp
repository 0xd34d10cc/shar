#pragma once

#include <cstdint> // u8
#include <cstdlib> // usize
#include <memory>  // std::unique_ptr
#include <tuple>   // std::tuple

#include "time.hpp"
#include "size.hpp"
#include "codec/convert.hpp"


extern "C" {
struct AVFrame;
void av_frame_free(AVFrame** frame);
}

namespace shar::codec::ffmpeg {

class Frame {
public:
  Frame() = default;
  Frame(const Frame&) = delete;
  Frame(Frame&&) noexcept = default;
  Frame& operator=(const Frame&) = delete;
  Frame& operator=(Frame&&) noexcept = default;
  ~Frame() = default;

  static Frame from_bgra(const char* data, Size size);
  static Frame alloc();
  Slice to_bgra() const;

  u8* data() noexcept;
  const u8* data() const noexcept;
  usize total_size() const noexcept;

  Size sizes() const noexcept;
  usize width() const noexcept;
  usize height() const noexcept;

  struct Channel {
    const u8* data{ nullptr };

    usize width{ 0 };
    usize height{ 0 };
    usize size { 0 };
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
  Frame(FramePtr frame, std::unique_ptr<u8[]> data);

  FramePtr m_frame;
  std::unique_ptr<u8[]> m_data;
  TimePoint m_time;
};

}