#include <iostream>
#include <chrono>
#include <mutex>
#include <array>

#include <ScreenCapture.h>

#include "image.hpp"
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
    std::lock_guard<std::mutex> guard(m_mutex);

    // FIXME
    if ((m_to + 1) % LIMIT == m_from) {
      throw std::runtime_error("Frame pipeline overflow");
    }

    m_frames[m_to].x_offset = frame.Bounds.left;
    m_frames[m_to].y_offset = frame.Bounds.top;
    m_frames[m_to].m_image.assign(frame);
    m_to = (m_to + 1) % LIMIT;
  }

  FrameUpdate* next_frame() {
    std::lock_guard<std::mutex> guard(m_mutex);
    if (m_to == m_from) {
      return nullptr;
    }

    return &m_frames[m_from];
  }

  void consume(std::size_t amount) {
    std::lock_guard<std::mutex> guard(m_mutex);
    m_from = (m_from + amount) % LIMIT;
  }

private:
  std::mutex m_mutex;
  std::size_t m_from;
  std::size_t m_to;
  std::array<FrameUpdate, LIMIT> m_frames;
};


int main() {

  // initializes opengl context
  // TODO: decouple OpenGL context initialization from window initialization
  shar::Window window;
  FramePipeline<120> pipeline;

  std::cout << "OpenGL " << glGetString(GL_VERSION) << std::endl;
  glEnable(GL_TEXTURE_2D);

  auto config = SL::Screen_Capture::CreateCaptureConfiguration([=](){
    // TODO: make it configurable
    auto monitor = SL::Screen_Capture::GetMonitors().front();
    return std::vector<Monitor>{monitor};
  });
 
  config->onFrameChanged(
    [&pipeline, timer = shar::Timer{ std::chrono::seconds(1) }, fps = 0]
    (const Image& image, const Monitor& monitor) mutable {
    
    if (timer.expired()) {
      std::cout << "ups: " << fps << std::endl;
      fps = 0;
      timer.restart();
    }
    ++fps;

    pipeline.push(image);
  });

  auto capture = config->start_capturing();
  
  const int fps = 60;
  auto interval = std::chrono::milliseconds(std::chrono::seconds(1)) / fps;
  capture->setFrameChangeInterval(interval);

  shar::Texture texture;
  texture.bind();

  // consumer should be faster
  shar::Timer timer{interval / 2};
  while (!window.should_close()) {
    timer.wait();
    timer.restart();
    
    if (auto frame = pipeline.next_frame()) {
      texture.update(frame->x_offset,        frame->y_offset, 
                     frame->m_image.width(), frame->m_image.height(), 
                     frame->m_image.bytes());
      pipeline.consume(1);      

      window.draw_texture(texture);
      window.swap_buffers();
    }

    window.poll_events();
  }
  return 0;
}