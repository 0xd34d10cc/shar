#include <iostream>
#include <chrono>

#include <ScreenCapture.h>

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

struct FrameProvider {
  FrameProvider(shar::FixedSizeQueue<FrameUpdate, 120>& pipeline)
    : m_pipeline(pipeline)
    , m_timer(std::chrono::seconds(1))
    , m_ups(0)
  {}

  void operator()(const Image& image, const Monitor&) {
    if (m_timer.expired()) {
      std::cout << "ups: " << m_ups << std::endl;
      m_ups = 0;
      m_timer.restart();
    }
    ++m_ups;

    shar::Image img{};
    img = image;
    m_pipeline.push(FrameUpdate{ 
      static_cast<std::size_t>(image.Bounds.left), 
      static_cast<std::size_t>(image.Bounds.top), 
      std::move(img)
    });
  }

  shar::FixedSizeQueue<FrameUpdate, 120>& m_pipeline;
  shar::Timer m_timer;
  std::size_t m_ups;
};


int main() {
  // TODO: make it configurable
  auto monitor = SL::Screen_Capture::GetMonitors().front();
  std::size_t width = static_cast<std::size_t>(monitor.Width);
  std::size_t height = static_cast<std::size_t>(monitor.Height);
  std::cout << "Capturing " << monitor.Name << " " << width << 'x' << height << std::endl;

  // initializes opengl context
  shar::Window window{ width, height };
  shar::FixedSizeQueue<FrameUpdate, 120> pipeline;
  
  shar::PacketSender sender;
  auto network_thread = std::thread([&]{
    sender.run();
  });

  std::cout << "OpenGL " << glGetString(GL_VERSION) << std::endl;
  glEnable(GL_TEXTURE_2D);

  shar::Encoder encoder{};
  shar::Decoder decoder{};
  shar::Image current_frame{};

  auto config = SL::Screen_Capture::CreateCaptureConfiguration([=]() {
    return std::vector<Monitor>{monitor};
  });

  config->onFrameChanged(FrameProvider(pipeline));
  config->onNewFrame([&](const Image& image, const Monitor&) {
    current_frame = image;
    auto data = encoder.encode(current_frame);
    if (!data.empty()){
      for (auto& packet: data) {
        sender.send(std::move(packet));
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
  });
  
  auto capture = config->start_capturing();
  
  const int fps = 60;
  auto interval = std::chrono::milliseconds(std::chrono::seconds(1)) / fps;
  capture->setFrameChangeInterval(interval);

  shar::Texture texture{ width, height };
  texture.bind();

  shar::Timer timer{ std::chrono::milliseconds(1) };
  while (!window.should_close()) {
    timer.wait();
    timer.restart();

    if (!pipeline.empty()) {
      // FIXME: this loop can apply updates from different frames
      do {
        FrameUpdate* frame_update = pipeline.get_next();
        texture.update(frame_update->x_offset,        frame_update->y_offset,
                       frame_update->m_image.width(), frame_update->m_image.height(),
                       frame_update->m_image.bytes());
        pipeline.consume(1);
      } while (!pipeline.empty());
      
      window.draw_texture(texture);
      window.swap_buffers();
    }

    window.poll_events();
  }

  sender.stop();
  network_thread.join();

  return 0;
}