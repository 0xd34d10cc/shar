#include <iostream>
#include <chrono>

#include <ScreenCapture.h>
#include <internal/SCCommon.h>

#include "image.hpp"
#include "encoder.hpp"
#include "decoder.hpp"
#include "window.hpp"
#include "texture.hpp"
#include "timer.hpp"
#include "queue.hpp"
#include "packet_sender.hpp"


using SL::Screen_Capture::Image;
using SL::Screen_Capture::Monitor;

struct FrameUpdate {
  FrameUpdate() = default;

  FrameUpdate(const FrameUpdate&) = delete;

  FrameUpdate(FrameUpdate&&) = default;

  FrameUpdate& operator=(FrameUpdate&&) = default;

  std::size_t x_offset;
  std::size_t y_offset;
  shar::Image m_image;
};

using FramePipeline = shar::FixedSizeQueue<FrameUpdate, 120>;

struct FrameProvider {
  FrameProvider(FramePipeline& pipeline)
      : m_pipeline(pipeline)
        , m_timer(std::chrono::seconds(1))
        , m_ups(0) {}

  FrameProvider(const FrameProvider&) = delete;

  FrameProvider(FrameProvider&&) = default;

  void operator()(const Image& image, const Monitor&) {
    if (m_timer.expired()) {
      std::cout << "ups: " << m_ups << std::endl;
      m_ups = 0;
      m_timer.restart();
    }
    ++m_ups;

    shar::Image img {};
    img = image;
    m_pipeline.push(FrameUpdate {
        static_cast<std::size_t>(OffsetX(image)),
        static_cast<std::size_t>(OffsetY(image)),
        std::move(img)
    });
  }

  FramePipeline& m_pipeline;
  shar::Timer m_timer;
  std::size_t m_ups;
};


struct FrameEncoder {
  FrameEncoder(shar::PacketSender& sender)
      : m_sender(sender)
        , m_current_frame()
        , m_encoder() {}

  FrameEncoder(const FrameEncoder&) = delete;

  FrameEncoder(FrameEncoder&& from) = default;

  void operator()(const Image& image, const Monitor&) {
    m_current_frame = image;
    auto data = m_encoder.encode(m_current_frame);
    if (!data.empty()) {
      for (auto& packet: data) {
        m_sender.send(std::move(packet));
        // decoder.push_packet(std::move(packet));
      }

      // bool more = true;
      // bool success = true;
      // while (more && success) {
      //   success = decoder.decode(more);
      //   if (auto image = decoder.pop_image()) {
      //     // pipeline.push(*image);
      //   }

      //   decoder.print_warnings();
      // }
    }
  }

  shar::PacketSender& m_sender;
  shar::Image   m_current_frame;
  shar::Encoder m_encoder;
};

template<typename H>
struct SharedFrameHandler {
  void operator()(const Image& image, const Monitor& monitor) {
    m_handler->operator()(image, monitor);
  }

  std::shared_ptr<H> m_handler;
};

static void event_loop(shar::Window& window, FramePipeline& pipeline) {
  shar::Texture texture {window.width(), window.height()};
  texture.bind();

  shar::Timer timer {std::chrono::milliseconds(1)};
  while (!window.should_close()) {
    timer.wait();
    timer.restart();

    if (!pipeline.empty()) {
      // FIXME: this loop can apply updates from different frames
      do {
        FrameUpdate* frame_update = pipeline.get_next();
        texture.update(frame_update->x_offset, frame_update->y_offset,
                       frame_update->m_image.width(), frame_update->m_image.height(),
                       frame_update->m_image.bytes());
        pipeline.consume(1);
      } while (!pipeline.empty());

      window.draw_texture(texture);
      window.swap_buffers();
    }

    window.poll_events();
  }
}

static shar::Window create_window(std::size_t width, std::size_t height) {
  // initializes opengl context
  return shar::Window {width, height};
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
  config->onFrameChanged(handler);
  return config;
}


int main() {
  FramePipeline pipeline;

  // TODO: make it configurable
  auto        monitor = SL::Screen_Capture::GetMonitors().front();
  std::size_t width   = static_cast<std::size_t>(monitor.Width);
  std::size_t height  = static_cast<std::size_t>(monitor.Height);
  std::cout << "Capturing " << monitor.Name << " " << width << 'x' << height << std::endl;

  auto               window = create_window(width, height);
  shar::PacketSender sender;
  // auto network_thread = std::thread([&]{
  // sender.run();
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