#include "display.hpp"


namespace shar::ui {

Display::Display(Context context, Window& window)
    : Context(std::move(context))
    , m_fps({"FPS", "frames per second", "count"}, m_registry)
    , m_window(window)
    , m_texture(window.size()) {}


void Display::setup() {
  m_texture.bind();
}

void Display::teardown() {
  m_texture.unbind();
}

void Display::stop() {

}

void Display::process(Frame frame) {
  m_fps.increment();
  m_texture.update(Point::origin(),
                   frame.size(),
                   frame.bytes());
  m_window.draw_texture(m_texture);
  m_window.swap_buffers();
  m_window.poll_events();

  if (m_window.should_close()) {
    stop();
    return;
  }
}

}