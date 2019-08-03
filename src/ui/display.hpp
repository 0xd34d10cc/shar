#pragma once

#include "context.hpp"
#include "channel.hpp"
#include "cancellation.hpp"
#include "metrics/gauge.hpp"
#include "codec/ffmpeg/frame.hpp"
#include "window.hpp"
#include "renderer.hpp"
#include "state.hpp"
#include "texture.hpp"


namespace shar::ui {

class Display: protected Context {
public:
  Display(Context context, Size size);
  Display(Display&&) = default;
  Display& operator=(Display&&) = default;
  ~Display() = default;

  void run(Receiver<codec::ffmpeg::Frame> frames);
  void shutdown();

private:
  void process_input();
  void process_gui();
  void render();

  bool m_gui_enabled{ true };

  Window m_window;
  Renderer m_renderer;
  State m_state;

  Texture m_texture;

  Cancellation m_running;
  metrics::Gauge m_fps;
};

}