#include "processors/screen_capture.hpp"


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

  return shar::Image {std::move(bytes), size};
}


struct FrameHandler {
  FrameHandler(shar::FramesSender& frames_consumer)
      : m_metrics_timer(std::chrono::seconds(1))
      , m_fps(0)
      , m_frames_consumer(frames_consumer) {}

  void operator()(const sc::Image& frame, const sc::Monitor& /* monitor */) {
    if (m_metrics_timer.expired()) {
//      std::cout << "ScreenCapture::fps = " << m_fps << std::endl;
      m_fps = 0;
      m_metrics_timer.restart();
    }
    ++m_fps;

    shar::Image buffer = convert(frame);
    // ignore return value here, if channel was disconnected ScreenCapture will stop
    // processing new frames anyway
    m_frames_consumer.send(std::move(buffer));
  }

  shar::Timer         m_metrics_timer;
  std::size_t         m_fps;
  shar::FramesSender& m_frames_consumer;
};

}

namespace shar {

ScreenCapture::ScreenCapture(const Milliseconds& interval,
                             const sc::Monitor& monitor,
                             Logger logger,
                             FramesSender output)
    : Source("ScreenCapture", logger, std::move(output))
    , m_interval(interval)
    , m_wakeup_timer(std::chrono::seconds(1))
    , m_config(nullptr)
    , m_capture(nullptr) {

  m_config = sc::CreateCaptureConfiguration([&] {
    return std::vector<sc::Monitor> {monitor};
  });

  m_config->onNewFrame(FrameHandler {Processor::output()});
}

void ScreenCapture::process(FalseInput) {
  m_wakeup_timer.restart();
  m_wakeup_timer.wait();
}

void ScreenCapture::setup() {
  m_capture = m_config->start_capturing();
  m_capture->setFrameChangeInterval(m_interval);
}

void ScreenCapture::teardown() {
  m_capture.reset();
}

}
