#pragma once

#include <cstddef>

#include "disable_warnings_push.hpp"
#include <ScreenCapture.h>
#include "disable_warnings_pop.hpp"

#include "channels/bounded.hpp"
#include "processors/source.hpp"
#include "primitives/timer.hpp"
#include "primitives/image.hpp"


namespace sc = SL::Screen_Capture;

using CaptureConfigPtr = std::shared_ptr<
    sc::ICaptureConfiguration<sc::ScreenCaptureCallback>
>;
using CaptureManagerPtr = std::shared_ptr<sc::IScreenCaptureManager>;
using Milliseconds = std::chrono::milliseconds;

namespace shar {

using FramesSender = channel::Sender<Image>;

class ScreenCapture : public Source<ScreenCapture, FramesSender> {
public:
  ScreenCapture(const Milliseconds& interval,
                const sc::Monitor& monitor,
                Logger logger,
                MetricsPtr metrics,
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