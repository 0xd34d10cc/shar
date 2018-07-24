#pragma once

#include <cstddef>

#include "disable_warnings_push.hpp"
#include <ScreenCapture.h>
#include "disable_warnings_pop.hpp"

#include "queues/frames_queue.hpp"
#include "processors/source.hpp"
#include "primitives/timer.hpp"


namespace sc = SL::Screen_Capture;

using CaptureConfigPtr = std::shared_ptr<
    sc::ICaptureConfiguration<sc::ScreenCaptureCallback>
>;
using CaptureManagerPtr = std::shared_ptr<sc::IScreenCaptureManager>;
using Milliseconds = std::chrono::milliseconds;

namespace shar {

class ScreenCapture : public Source<ScreenCapture, FramesQueue> {
public:
  ScreenCapture(const Milliseconds& interval,
                const sc::Monitor& monitor,
                Logger& logger,
                FramesQueue& output);
  ScreenCapture(const ScreenCapture&) = delete;
  ScreenCapture(ScreenCapture&&) = delete;

  void setup();
  void process(Void*);
  void teardown();

private:
  Milliseconds m_interval;
  shar::Timer  m_wakeup_timer;

  CaptureConfigPtr  m_config;
  CaptureManagerPtr m_capture;
};

}