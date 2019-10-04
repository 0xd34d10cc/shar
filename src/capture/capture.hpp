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