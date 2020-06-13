#pragma once

#include <memory>
#include <optional>

#include "disable_warnings_push.hpp"
#include <ScreenCapture.h>
#include "disable_warnings_pop.hpp"

#include "context.hpp"
#include "channel.hpp"
#include "time.hpp"
#include "size.hpp"
#include "codec/ffmpeg/frame.hpp"


namespace sc = SL::Screen_Capture;

using CaptureConfigPtr = std::shared_ptr<
    sc::ICaptureConfiguration<sc::ScreenCaptureCallback>
>;
using CaptureManagerPtr = std::shared_ptr<sc::IScreenCaptureManager>;

namespace shar {

struct BGRAFrame {
  std::unique_ptr<u8[]> data{ nullptr };
  Size size{ Size::empty() };

  BGRAFrame clone() const {
    auto height = size.height();
    auto width = size.width();
    auto n = height * width * 4;
    auto new_frame = BGRAFrame{};
    new_frame.data = std::make_unique<u8[]>(n);
    memcpy(new_frame.data.get(), data.get(), n);
    new_frame.size = Size{ height, width };
    return new_frame;
  }
};

class Capture : protected Context {
public:
  Capture(Context context,
          Milliseconds interval,
          sc::Monitor monitor);
  Capture(const Capture&) = delete;
  Capture(Capture&&) = default;
  Capture& operator=(const Capture&) = delete;
  Capture& operator=(Capture&&) noexcept = default;
  ~Capture() = default;

  void run(Sender<codec::ffmpeg::Frame> output,
           std::optional<Sender<BGRAFrame>> bgra_out);
  void shutdown();

private:
  Milliseconds      m_interval;
  CaptureConfigPtr  m_capture_config;
  CaptureManagerPtr m_capture;
};

}