#include "display.hpp"

#include "codec/convert.hpp"


namespace shar::ui {

Display::Display(Context context, Window& window)
    : Context(std::move(context))
    , m_fps({"FPS", "frames per second", "count"}, m_registry)
    , m_window(window)
    , m_texture(window.size()) {}


void Display::run(Receiver<codec::ffmpeg::Frame> frames) {
  m_texture.bind();

  while (!m_running.expired() && frames.connected()) {
    auto frame = frames.receive();
    if (!frame) {
      // end of stream
      break;
    }

    auto bgra = frame->to_bgra();

    m_fps.increment();
    m_texture.update(Point::origin(),
                     frame->sizes(),
                     bgra.data.get());
    m_window.draw_texture(m_texture);
    m_window.swap_buffers();
    m_window.poll_events();

    if (m_window.should_close()) {
      break;
    }
  }

  m_texture.unbind();
}

}