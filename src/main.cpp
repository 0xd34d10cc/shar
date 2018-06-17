#include <iostream>
#include <chrono>

#include "disable_warnings_push.hpp"
#include <ScreenCapture.h>
#include <internal/SCCommon.h>
#include "disable_warnings_pop.hpp"

#include "image.hpp"
#include "window.hpp"
#include "texture.hpp"
#include "timer.hpp"
#include "queue.hpp"
#include "packet_sender.hpp"
#include "encoder.hpp"
#include "decoder.hpp"
#include "point.hpp"

using SL::Screen_Capture::Image;
using SL::Screen_Capture::Monitor;

class FrameUpdate {
public:
  FrameUpdate()
      : m_offset(shar::Point::origin())
      , m_image() {}

  FrameUpdate(shar::Point offset, shar::Image image)
      : m_offset(offset)
      , m_image(std::move(image)) {}

  FrameUpdate(const FrameUpdate&) = delete;

  FrameUpdate(FrameUpdate&&) = default;

  FrameUpdate& operator=(FrameUpdate&&) = default;

  const shar::Point& offset() const {
    return m_offset;
  }

  const shar::Image& image() const {
    return m_image;
  }

private:
  shar::Point m_offset;
  shar::Image m_image;
};

using FramePipeline = shar::FixedSizeQueue<FrameUpdate, 120>;

struct FrameProvider {
  FrameProvider(FramePipeline& pipeline)
      : m_pipeline(pipeline)
      , m_timer(std::chrono::seconds(1))
      , m_ups(0)
      , m_bps(0)
      , m_fps(0)
      , m_encoder(shar::Size{1080, 1920}, 5000000)
      , m_decoder() {}

  FrameProvider(const FrameProvider&) = delete;

  FrameProvider(FrameProvider&&) = default;

  void operator()(const Image& image, const Monitor&) {
    if (m_timer.expired()) {
      std::cout << "ups: " << m_ups << std::endl;
      std::cout << "bps: " << m_bps << std::endl;
      std::cout << "fps: " << m_fps << std::endl;

      m_ups = 0;
      m_bps = 0;
      m_fps = 0;

      m_timer.restart();
    }
    ++m_ups;

    shar::Image img {};
    img = image;

    m_pipeline.push(FrameUpdate{
      shar::Point::origin(),
      std::move(img)
    });
//
//    auto packets = m_encoder.encode(img);
//    for (const auto& packet: packets) {
//      m_bps += packet.size();
//
//      auto frame = m_decoder.decode(packet);
//      if (!frame.empty()) {
//        m_fps += 1;
//        m_pipeline.push(FrameUpdate {
//            shar::Point::origin(), // offset
//            std::move(frame)
//        });
//      }
//    }

  }

  // output channel
  FramePipeline& m_pipeline;

  // metrics
  shar::Timer m_timer;
  std::size_t m_ups;
  std::size_t m_bps;
  std::size_t m_fps;

  // codec stuff
  shar::Encoder m_encoder;
  shar::Decoder m_decoder;
};

template<typename H>
struct SharedFrameHandler {
  void operator()(const Image& image, const Monitor& monitor) {
    m_handler->operator()(image, monitor);
  }

  std::shared_ptr<H> m_handler;
};

static void event_loop(shar::Window& window, FramePipeline& pipeline) {
  shar::Texture texture {window.size()};
  texture.bind();

  shar::Timer timer {std::chrono::milliseconds(1)};
  while (!window.should_close()) {
    timer.wait();
    timer.restart();

    if (!pipeline.empty()) {
      // FIXME: this loop can apply updates from different frames
      do {
        FrameUpdate* update = pipeline.get_next();
        texture.update(update->offset(),
                       update->image().size(),
                       update->image().bytes());
        pipeline.consume(1);
      } while (!pipeline.empty());

      window.draw_texture(texture);
      window.swap_buffers();
    }

    window.poll_events();
  }
}

using CaptureConfigPtr = std::shared_ptr<
    SL::Screen_Capture::ICaptureConfiguration<
        SL::Screen_Capture::ScreenCaptureCallback>>;

static CaptureConfigPtr create_capture_configuration(const Monitor& monitor,
                                                     FramePipeline& pipeline,
                                                     shar::PacketSender& sender) {
  auto config  = SL::Screen_Capture::CreateCaptureConfiguration([&] {
    return std::vector<Monitor> {monitor};
  });
  auto handler = SharedFrameHandler<FrameProvider> {std::make_shared<FrameProvider>(pipeline)};
  config->onNewFrame(handler);
  return config;
}


int main() {
  FramePipeline pipeline;

  // TODO: make it configurable
  auto        monitor = SL::Screen_Capture::GetMonitors().front();
  std::size_t width   = static_cast<std::size_t>(monitor.Width);
  std::size_t height  = static_cast<std::size_t>(monitor.Height);
  std::cout << "Capturing " << monitor.Name << " " << width << 'x' << height << std::endl;

  auto window = shar::Window {shar::Size {height, width}};;
  shar::PacketSender sender;
  // auto network_thread = std::thread([&]{
  //  sender.run();
  // });

  auto config  = create_capture_configuration(monitor, pipeline, sender);
  auto capture = config->start_capturing();

  const int fps      = 60;
  auto      interval = std::chrono::milliseconds(std::chrono::seconds(1)) / fps;
  capture->setFrameChangeInterval(interval);

  event_loop(window, pipeline);

  // sender.stop();
  // network_thread.join();

  return 0;
}