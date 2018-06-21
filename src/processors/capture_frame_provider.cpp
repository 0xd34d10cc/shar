#include <iostream>

#include "processors/capture_frame_provider.hpp"


namespace {

shar::Image convert(const sc::Image& image) noexcept {
  std::size_t width  = static_cast<std::size_t>(Width(image));
  std::size_t height = static_cast<std::size_t>(Height(image));
  std::size_t pixels = width * height;

  const std::size_t PIXEL_SIZE = 4;

  auto bytes = std::make_unique<uint8_t[]>(pixels * PIXEL_SIZE);
  auto size  = shar::Size {height, width};

  assert(bytes.get() != nullptr);
  sc::Extract(image, bytes.get(), pixels * PIXEL_SIZE);

  return shar::Image{std::move(bytes), size};
}


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

    shar::Image buffer = convert(frame) ;
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
    : Processor("CaptureFrameProvider")
    , m_interval(interval)
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
