#include "display.hpp"


namespace shar {

Display::Display(Context context, Window& window, Receiver<Frame> input)
    : Base(std::move(context), std::move(input))
    , m_fps_metric()
    , m_window(window)
    , m_texture(window.size()) {}


void Display::setup() {
  m_fps_metric = m_metrics->add("Display\tfps", Metrics::Format::Count);
  m_texture.bind();
}

void Display::teardown() {
  m_texture.unbind();
  m_metrics->remove(m_fps_metric);
}

void Display::process(Frame frame) {
  m_metrics->increase(m_fps_metric, 1);
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