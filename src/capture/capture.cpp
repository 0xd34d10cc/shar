#include "capture/capture.hpp"


namespace {

using namespace shar;
using Frame = codec::ffmpeg::Frame;

static const usize NCHANNELS = 4; // bgra

Frame convert(const sc::Image& image) noexcept {
  const auto width  = static_cast<usize>(Width(image));
  const auto height = static_cast<usize>(Height(image));
  auto size  = Size{ height, width };

  // Frame::from_bgra expects no padding, for now
  if (sc::isDataContiguous(image)) {
    const char* data = reinterpret_cast<const char*>(sc::StartSrc(image));
    return Frame::from_bgra(data, size);
  } else {
    // do a bunch of memcpy operations to get rid of padding
    // TODO: support padding in bgra_to_yuv to avoid extra allocation + copy
    usize nbytes = width * height * NCHANNELS;
    const auto data = std::make_unique<u8[]>(nbytes);
    sc::Extract(image, data.get(), nbytes);
    return Frame::from_bgra(reinterpret_cast<const char*>(data.get()), size);
  }
}

BGRAFrame to_bgra(const sc::Image& image) noexcept {
  BGRAFrame frame;

  const auto width = static_cast<usize>(Width(image));
  const auto height = static_cast<usize>(Height(image));
  usize n = width * height * NCHANNELS;

  frame.data = std::make_unique<u8[]>(n);
  frame.size = Size{ height, width };

  sc::Extract(image, frame.data.get(), n);
  return frame;
}

struct FrameHandler {
  explicit FrameHandler(std::shared_ptr<Sender<Frame>> consumer,
                        std::shared_ptr<Sender<BGRAFrame>> bgra_sender)
      : m_consumer(std::move(consumer))
      , m_bgra_consumer(std::move(bgra_sender))
      , m_cursor_data(std::make_shared<CursorData>())
      {}

  void operator()(const sc::Image& buffer, const sc::Monitor& /* monitor */) {
    const sc::ImageBGRA* current = sc::StartSrc(buffer);
    if (m_cursor_data->cursor) {
      std::lock_guard<std::mutex> lock(m_cursor_data->mtx);

      // go to cursor starting location
      for (auto i = 0; i < m_cursor_data->last_position.y; i++) {
        current = sc::GotoNextRow(buffer, current);
      }

      auto y_to_draw = m_cursor_data->height;
      if (m_cursor_data->last_position.y + m_cursor_data->height >
          Height(buffer)) {
        if (m_cursor_data->last_position.y > Height(buffer)) {
          y_to_draw = 0;
        } else {
          y_to_draw = Height(buffer) - m_cursor_data->last_position.y;
        }       
      }

      auto x_to_draw = m_cursor_data->width;
      if (m_cursor_data->last_position.x + m_cursor_data->width >
          Width(buffer)) {
        if (m_cursor_data->last_position.x > Width(buffer)) {
          x_to_draw = 0; 
        } else {
          x_to_draw = Width(buffer) - m_cursor_data->last_position.x;
        }
      }

      for (auto i = 0; i < y_to_draw; i++) {
        auto* row_to_change = const_cast<sc::ImageBGRA*>(current);
        std::memcpy(row_to_change + m_cursor_data->last_position.x,
                    m_cursor_data->cursor.get() + m_cursor_data->width*i,
                    x_to_draw);
        current = sc::GotoNextRow(buffer, current);
      }
    }
     
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

  void operator()(const sc::Image* img, const sc::MousePoint& point) {
    std::lock_guard<std::mutex> lock(m_cursor_data->mtx);

    if (img) {
      const auto width = static_cast<usize>(Width(*img));
      const auto height = static_cast<usize>(Height(*img));
      const auto size = width * height;
      if (!m_cursor_data->cursor) {
        m_cursor_data->cursor = std::make_unique<sc::ImageBGRA[]>(size);
      }

      if (size > (m_cursor_data->width * m_cursor_data->height)) {
        m_cursor_data->cursor = std::make_unique<sc::ImageBGRA[]>(size);
      }

      m_cursor_data->width = width;
      m_cursor_data->height = height;

      sc::Extract(*img,
                  reinterpret_cast<u8*>(m_cursor_data->cursor.get()),
                  size * NCHANNELS);
    }

    m_cursor_data->last_position = point.Position;
  }

  // shared_ptr is used here because FrameHandler has to be copyable
  // onNewFrame accepts handler by const reference
  std::shared_ptr<Sender<Frame>> m_consumer;
  std::shared_ptr<Sender<BGRAFrame>> m_bgra_consumer;

  // mouse control block
  struct CursorData {
    usize width = 0;
    usize height = 0;
    sc::Point last_position = {0, 0};
    std::unique_ptr<sc::ImageBGRA[]> cursor;
    std::mutex mtx;
  };

  std::shared_ptr<CursorData> m_cursor_data;
};

}

namespace shar {

Capture::Capture(Context context,
                 Milliseconds interval,
                 sc::Monitor monitor)
    : Context(std::move(context))
    , m_interval(interval)
    , m_capture(nullptr) {
  usize id = static_cast<usize>(monitor.Id);
  m_capture_config = sc::CreateCaptureConfiguration([id]() mutable {
    const auto monitors = sc::GetMonitors();
    if (monitors.empty()) {
      return std::vector<sc::Monitor>();
    }

    return std::vector<sc::Monitor>{ monitors[std::min(id, monitors.size() - 1)] };
  });

}

void Capture::run(Sender<Frame> output,
                  std::optional<Sender<BGRAFrame>> bgra_output) {
  // handled has to be copyable...
  auto sender = std::make_shared<Sender<Frame>>(std::move(output));
  auto bgra_sender = bgra_output
    ? std::make_shared<Sender<BGRAFrame>>(std::move(*bgra_output))
    : std::shared_ptr<Sender<BGRAFrame>>();
  
  auto frame_handler = FrameHandler{std::move(sender), std::move(bgra_sender)};
  m_capture_config->onNewFrame(frame_handler);
  m_capture_config->onMouseChanged(frame_handler);
  m_capture = m_capture_config->start_capturing();

  m_capture->setMouseChangeInterval(m_interval);
  m_capture->setFrameChangeInterval(m_interval);
}

void Capture::shutdown() {
  m_capture_config.reset();
  m_capture.reset();
}

}
