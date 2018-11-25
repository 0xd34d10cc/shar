#include "capture/capture.hpp"


namespace {

using namespace shar;

Frame convert(const sc::Image& image) noexcept {
  const auto width  = static_cast<std::size_t>(Width(image));
  const auto height = static_cast<std::size_t>(Height(image));
  const std::size_t pixels = width * height;

  const std::size_t PIXEL_SIZE = 4;

  auto bytes = std::make_unique<uint8_t[]>(pixels * PIXEL_SIZE);
  auto size  = Size {height, width};

  assert(bytes != nullptr);
  sc::Extract(image, bytes.get(), pixels * PIXEL_SIZE);

  return Frame {std::move(bytes), size};
}


struct FrameHandler {
  explicit FrameHandler(std::shared_ptr<Sender<Frame>> consumer)
      : m_consumer(std::move(consumer)) {}

  void operator()(const sc::Image& frame, const sc::Monitor& /* monitor */) {
    Frame buffer = convert(frame);

    // ignore return value here, 
    // if channel was disconnected ScreenCapture will stop
    // processing new frames anyway
    m_consumer->send(std::move(buffer));
  }

  std::shared_ptr<Sender<Frame>> m_consumer;
};

}

namespace shar {

Capture::Capture(Context context,
                 Milliseconds interval,
                 sc::Monitor monitor)
    : Context(std::move(context))
    , m_interval(interval)
    , m_capture(nullptr) {
  m_capture_config = sc::CreateCaptureConfiguration([=] {
    return std::vector<sc::Monitor> {monitor};
  });

}

void Capture::run(Sender<Frame> output) {
  auto sender = std::make_shared<Sender<Frame>>(std::move(output));
  m_capture_config->onNewFrame(FrameHandler{std::move(sender)});
  m_capture = m_capture_config->start_capturing();
  m_capture->setFrameChangeInterval(m_interval);
}

void Capture::shutdown() {
  m_capture_config.reset();
  m_capture.reset();
}

}
