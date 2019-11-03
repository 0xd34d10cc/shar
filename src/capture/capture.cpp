#include "capture/capture.hpp"


namespace {

using namespace shar;
using Frame = codec::ffmpeg::Frame;

Frame convert(const sc::Image& image) noexcept {
  const auto width  = static_cast<usize>(Width(image));
  const auto height = static_cast<usize>(Height(image));
  auto size  = Size{ height, width };

  // Frame::from_bgra expects no padding, for now
  // TODO: support padded images
  assert(sc::isDataContiguous(image));
  const char* data = reinterpret_cast<const char*>(sc::StartSrc(image));
  return Frame::from_bgra(data, size);
}

BGRAFrame to_bgra(const sc::Image& image) noexcept {
  BGRAFrame frame;

  const auto width = static_cast<usize>(Width(image));
  const auto height = static_cast<usize>(Height(image));
  usize n = width * height * 4;

  frame.data = std::make_unique<u8[]>(n);
  frame.size = Size{ height, width };

  assert(sc::isDataContiguous(image));
  std::memcpy(frame.data.get(), sc::StartSrc(image), n);
  return frame;
}

struct FrameHandler {
  explicit FrameHandler(std::shared_ptr<Sender<Frame>> consumer,
                        std::shared_ptr<Sender<BGRAFrame>> bgra_sender)
      : m_consumer(std::move(consumer))
      , m_bgra_consumer(std::move(bgra_sender))
      {}

  void operator()(const sc::Image& buffer, const sc::Monitor& /* monitor */) {
    Frame frame = convert(buffer);
    frame.set_timestamp(Clock::now());

    // TODO: remove
    if (m_bgra_consumer) {
      m_bgra_consumer->try_send(to_bgra(buffer));
    }

    // ignore return value here,
    // if channel was disconnected ScreenCapture will stop
    // processing new frames anyway
    m_consumer->try_send(std::move(frame));
  }

  // shared_ptr is used here because FrameHandler has to be copyable
  // onNewFrame accepts handler by const reference
  std::shared_ptr<Sender<Frame>> m_consumer;
  std::shared_ptr<Sender<BGRAFrame>> m_bgra_consumer;
};

}

namespace shar {

Capture::Capture(Context context,
                 Milliseconds interval,
                 sc::Monitor monitor)
    : Context(std::move(context))
    , m_interval(interval)
    , m_capture(nullptr) {
  m_capture_config = sc::CreateCaptureConfiguration([m{std::move(monitor)}]() mutable {
    return std::vector<sc::Monitor>{ std::move(m) };
  });

}

void Capture::run(Sender<Frame> output,
                  std::optional<Sender<BGRAFrame>> bgra_output) {
  // handled has to be copyable...
  auto sender = std::make_shared<Sender<Frame>>(std::move(output));
  auto bgra_sender = bgra_output
    ? std::make_shared<Sender<BGRAFrame>>(std::move(*bgra_output))
    : std::shared_ptr<Sender<BGRAFrame>>();

  m_capture_config->onNewFrame(FrameHandler{std::move(sender),
                                            std::move(bgra_sender)});
  m_capture = m_capture_config->start_capturing();
  m_capture->setFrameChangeInterval(m_interval);
}

void Capture::shutdown() {
  m_capture_config.reset();
  m_capture.reset();
}

}
