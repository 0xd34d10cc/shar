#include <iostream>
#include <chrono>

#include <ScreenCapture.h>
#include <internal/SCCommon.h>

#include "image.hpp"
#include "window.hpp"
#include "texture.hpp"
#include "timer.hpp"
#include "queue.hpp"
#include "packet_sender.hpp"
#include "encoder.hpp"
#include "decoder.hpp"


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


template<typename T>
const T& clamp(const T& v, const T& lo, const T& hi) {
  return v > hi ? hi :
         v < lo ? lo :
         v;
}

std::unique_ptr<uint8_t[]> yuv420p_to_bgra(const uint8_t* ys,
                                           const uint8_t* us,
                                           const uint8_t* vs,
                                           size_t height, size_t width) {
  size_t u_width = width / 2;
  size_t i = 0;

  auto bgra = std::make_unique<uint8_t[]>(height * width * 4);

  for (int line = 0; line < height; ++line) {
    for (int coll = 0; coll < width; ++coll) {

      uint8_t y = ys[line * width + coll];
      uint8_t u = us[(line / 2) * u_width + (coll / 2)];
      uint8_t v = vs[(line / 2) * u_width + (coll / 2)];

      int c = y - 16;
      int d = u - 128;
      int e = v - 128;

      uint8_t r = static_cast<uint8_t>(clamp((298 * c + 409 * e + 128) >> 8, 0, 255));
      uint8_t g = static_cast<uint8_t>(clamp((298 * c - 100 * d - 208 * e + 128) >> 8, 0, 255));
      uint8_t b = static_cast<uint8_t>(clamp((298 * c + 516 * d + 128) >> 8, 0, 255));

      bgra[i + 0] = b;
      bgra[i + 1] = g;
      bgra[i + 2] = r;
      bgra[i + 3] = 0;

      i += 4;
    }
  }

  return bgra;
}


using ChannelData = std::vector<uint8_t>;

std::array<ChannelData, 3> bgra_to_yuv420p(const shar::Image& image) {
  std::array<ChannelData, 3> channels;
  ChannelData& ys = channels[0];
  ChannelData& us = channels[1];
  ChannelData& vs = channels[2];

  ys.reserve(image.size());
  us.reserve(image.size() / 4);
  vs.reserve(image.size() / 4);

  const auto luma = [](uint8_t r, uint8_t g, uint8_t b) {
    return static_cast<uint8_t >(((66 * r + 129 * g + 25 * b) >> 8) + 16);
  };

  const uint8_t* raw_image = image.bytes();
  size_t i = 0;

  for (auto line = 0; line < image.height(); ++line) {
    if (line % 2 == 0) {
      for (auto x = 0; x < image.width(); x += 2) {
        uint8_t r = raw_image[4 * i + 2];
        uint8_t g = raw_image[4 * i + 1];
        uint8_t b = raw_image[4 * i];

        uint8_t y = luma(r, g, b);
        uint8_t u = static_cast<uint8_t >(((-38 * r + -74 * g + 112 * b) >> 8) + 128);
        uint8_t v = static_cast<uint8_t >(((112 * r + -94 * g + -18 * b) >> 8) + 128);

        ys.push_back(y);
        us.push_back(u);
        vs.push_back(v);

        i++;

        r = raw_image[4 * i + 2];
        g = raw_image[4 * i + 1];
        b = raw_image[4 * i];

        y = luma(r, g, b);
        ys.push_back(y);
        i++;
      }
    }
    else {
      for (size_t x = 0; x < image.width(); x += 1) {
        uint8_t r = raw_image[4 * i + 2];
        uint8_t g = raw_image[4 * i + 1];
        uint8_t b = raw_image[4 * i];

        uint8_t y = luma(r, g, b);
        ys.push_back(y);
        i++;
      }
    }
  }

  return channels;
}



using FramePipeline = shar::FixedSizeQueue<FrameUpdate, 120>;

struct FrameProvider {
  FrameProvider(FramePipeline& pipeline)
      : m_pipeline(pipeline)
      , m_timer(std::chrono::seconds(1))
      , m_ups(0)
      , m_bps(0)
      , m_fps(0)
      , m_encoder(20, 1920, 1080, 5000000)
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

    auto packets = m_encoder.encode(img);
    for (const auto& packet: packets) {
      m_bps += packet.size();

      auto decoded_frame = m_decoder.decode(packet);
      if (!decoded_frame.empty()) {
        m_fps += 1;
        m_pipeline.push(FrameUpdate {0, 0, std::move(decoded_frame)});
      }
    }

  }

  FramePipeline& m_pipeline;
  shar::Timer   m_timer;
  std::size_t   m_ups;
  std::size_t   m_bps;
  std::size_t   m_fps;
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

  auto               window = create_window(width, height);
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