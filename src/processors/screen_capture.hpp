#pragma once

#include <cstddef>

#include "disable_warnings_push.hpp"
#include <ScreenCapture.h>
#include "disable_warnings_pop.hpp"

#include "channels/bounded.hpp"
#include "processors/source.hpp"
#include "primitives/timer.hpp"
#include "primitives/frame.hpp"


namespace sc = SL::Screen_Capture;

using CaptureConfigPtr = std::shared_ptr<
    sc::ICaptureConfiguration<sc::ScreenCaptureCallback>
>;
using CaptureManagerPtr = std::shared_ptr<sc::IScreenCaptureManager>;
using Milliseconds = std::chrono::milliseconds;

namespace shar {

using FramesSender = channel::Sender<Frame>;

class ScreenCapture : public Source<ScreenCapture, FramesSender> {
public:
  using Base = Source<ScreenCapture, FramesSender>;
  using Context = typename Base::Context;

  ScreenCapture(Context context,
                Milliseconds interval,
                const sc::Monitor& monitor,
                FramesSender output);
  ScreenCapture(const ScreenCapture&) = delete;
  ScreenCapture(ScreenCapture&&) = default;

  void setup();
  void process(FalseInput);
  void teardown();

private:
  Milliseconds m_interval;
  shar::Timer  m_wakeup_timer;

  CaptureConfigPtr  m_config;
  CaptureManagerPtr m_capture;
};

}