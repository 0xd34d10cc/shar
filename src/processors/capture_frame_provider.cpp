#include <iostream>

//#include "disable_warnings_push.hpp"
//#include <ScreenCapture.h>
//#include <internal/SCCommon.h>
//#include "disable_warnings_pop.hpp"

#include "processors/capture_frame_provider.hpp"


namespace {

struct FrameHandler {
  FrameHandler(shar::FramesQueue& frames_consumer)
      : m_metrics_timer(std::chrono::seconds(1))
      , m_fps(0)
      , m_frames_consumer(frames_consumer) {}

  void operator()(const sc::Image& frame, const sc::Monitor& /* monitor */) {
    if (m_metrics_timer.expired()) {
      std::cout << "CaptureFrameProvider::fps = " << m_fps << std::endl;
      m_fps = 0;
      m_metrics_timer.restart();
    }
    ++m_fps;

    shar::Image buffer;
    buffer = frame; // convert sc::Image to shar::Image (memcpy)
    m_frames_consumer.push(std::move(buffer));
  }

  shar::Timer m_metrics_timer;
  std::size_t m_fps;
  shar::FramesQueue& m_frames_consumer;
};

}


namespace shar {

CaptureFrameProvider::CaptureFrameProvider(const Milliseconds& interval,
                                           const sc::Monitor& monitor,
                                           FramesQueue& output)
    : m_interval(interval)
    , m_wakeup_timer(std::chrono::seconds(1))
    , m_config(nullptr)
    , m_capture(nullptr) {

  m_config = sc::CreateCaptureConfiguration([&] {
    return std::vector<sc::Monitor> {monitor};
  });

  m_config->onNewFrame(FrameHandler {output});
}

void CaptureFrameProvider::run() {
  Processor::start();

  m_capture = m_config->start_capturing();
  m_capture->setFrameChangeInterval(m_interval);
  while (is_running()) {
    if (m_wakeup_timer.expired()) {
      m_wakeup_timer.restart();
    }
    m_wakeup_timer.wait();
  }

  m_capture.reset();
}

}
