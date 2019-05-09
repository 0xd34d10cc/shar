#pragma once

#include "window.hpp"
#include "texture.hpp"
#include "context.hpp"
#include "channel.hpp"
#include "cancellation.hpp"
#include "metrics/gauge.hpp"
#include "codec/ffmpeg/frame.hpp"


namespace shar::ui {

class Display: protected Context {
public:
  // NOTE: |window| should be initialized in same
  // thread from which run() will be called
  Display(Context context, Window& window);

  void run(Receiver<codec::ffmpeg::Frame> frames);
  void shutdown();

private:
  Cancellation m_running;
  metrics::Gauge m_fps;
  Window& m_window;
  Texture m_texture;
};

}