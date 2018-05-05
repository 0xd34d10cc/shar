#include <iostream>
#include <fstream>
#include <chrono>
#include <mutex>
#include <array>
#include <condition_variable>

#include <ScreenCapture.h>

#include "image.hpp"
#include "encoder.hpp"
#include "decoder.hpp"
#include "window.hpp"
#include "texture.hpp"
#include "timer.hpp"


using SL::Screen_Capture::Image;
using SL::Screen_Capture::Monitor;

struct FrameUpdate {
  std::size_t x_offset;
  std::size_t y_offset;
  shar::Image m_image;
};

template <std::size_t LIMIT>
class FramePipeline {
public:
  FramePipeline()
    : m_from(0)
    , m_to(0)
    , m_frames()
  {}

  void push(const SL::Screen_Capture::Image& frame) {
    std::unique_lock<std::mutex> lock(m_mutex);

    while ((m_to + 1) % LIMIT == m_from) {
      m_condvar.wait(lock);
    }

    m_frames[m_to].x_offset = frame.Bounds.left;
    m_frames[m_to].y_offset = frame.Bounds.top;
    m_frames[m_to].m_image.assign(frame);
    m_to = (m_to + 1) % LIMIT;
  }

  void push(shar::Image& image) {
    std::unique_lock<std::mutex> lock(m_mutex);

    while ((m_to + 1) % LIMIT == m_from) {
      m_condvar.wait(lock);
    }

    m_frames[m_to].x_offset = 0;
    m_frames[m_to].y_offset = 0;
    m_frames[m_to].m_image = std::move(image);
    m_to = (m_to + 1) % LIMIT;
  }

  FrameUpdate* next_frame() {
    std::unique_lock<std::mutex> lock(m_mutex);
   
    // TODO: provide function for consumer thread to wait till next frame arrives
    if (m_to == m_from) {
      return nullptr;
    }

    return &m_frames[m_from];
  }

  void consume(std::size_t amount) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_from = (m_from + amount) % LIMIT;
    m_condvar.notify_all();
  }

private:
  std::mutex m_mutex;
  std::condition_variable m_condvar;

  std::size_t m_from;
  std::size_t m_to;
  std::array<FrameUpdate, LIMIT> m_frames;
};

struct FrameProvider {
  FrameProvider(FramePipeline<120>& pipeline)
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

    m_pipeline.push(image);
  }

  FramePipeline<120>& m_pipeline;
  shar::Timer m_timer;
  std::size_t m_ups;
};


int main() {
  // TODO: make it configurable
  auto monitor = SL::Screen_Capture::GetMonitors().front();
  std::size_t width = static_cast<std::size_t>(monitor.Width);
  std::size_t height = static_cast<std::size_t>(monitor.Height);
  std::cout << "Capturing " << monitor.Name << " " << width << 'x' << height << std::endl;

  std::ofstream packets_file("out.h265", std::ios::trunc | std::ios::out | std::ios::binary);

  size_t count_packet = 0;

  // initializes opengl context
  shar::Window window{ width, height };
  FramePipeline<120> pipeline;

  std::cout << "OpenGL " << glGetString(GL_VERSION) << std::endl;
  glEnable(GL_TEXTURE_2D);

  shar::Encoder encoder{};
  auto header = encoder.gen_header();
  shar::Decoder decoder{};
  shar::Image current_frame{};


  auto config = SL::Screen_Capture::CreateCaptureConfiguration([=]() {
    return std::vector<Monitor>{monitor};
  });

  config->onFrameChanged(FrameProvider(pipeline));
  config->onNewFrame([&](const Image& image, const Monitor&) {
    current_frame.assign(image);
    auto data = encoder.encode(current_frame);
    if (!data.empty()){
      for (auto& packet: data) {
        decoder.push_packet(std::move(packet));
      }

      bool more = true;
      bool success = true;
      while (more && success) {
        success = decoder.decode(more);

        if (auto image = decoder.pop_image()) {
          // pipeline.push(*image);
        }

        decoder.print_warnings();
      }    
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

    if (auto* frame_update = pipeline.next_frame()) {

      // FIXME: this loop can apply updates from different frames
      do {
        texture.update(frame_update->x_offset,        frame_update->y_offset,
                       frame_update->m_image.width(), frame_update->m_image.height(),
                       frame_update->m_image.bytes());
        pipeline.consume(1);
        frame_update = pipeline.next_frame();
      } while (frame_update);
      
      window.draw_texture(texture);
      window.swap_buffers();
    }

    window.poll_events();
  }

  packets_file.flush();
  packets_file.close();
  return 0;
}