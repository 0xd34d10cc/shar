#pragma once

#include <cstddef>

#include "disable_warnings_push.hpp"
#include <ScreenCapture.h>
#include "disable_warnings_pop.hpp"

#include "context.hpp"
#include "channel.hpp"
#include "time.hpp"
#include "encoder/ffmpeg/frame.hpp"


namespace sc = SL::Screen_Capture;

using CaptureConfigPtr = std::shared_ptr<
    sc::ICaptureConfiguration<sc::ScreenCaptureCallback>
>;
using CaptureManagerPtr = std::shared_ptr<sc::IScreenCaptureManager>;

namespace shar {

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

  void run(Sender<encoder::ffmpeg::Frame> output);
  void shutdown();

private:
  Milliseconds      m_interval;
  CaptureConfigPtr  m_capture_config;
  CaptureManagerPtr m_capture;
};

}