#pragma once

#include "window.hpp"
#include "texture.hpp"
#include "point.hpp"
#include "size.hpp"
#include "context.hpp"
#include "frame.hpp"
#include "channel.hpp"
#include "metrics/gauge.hpp"


namespace shar::ui {

class Display: protected Context {
public:
  // NOTE: |window| should be initialized in same
  // thread from which run() will be called
  Display(Context context, Window& window);

  void setup();
  void teardown();
  void stop();

  void process(Frame frame);

private:
  metrics::Gauge m_fps;
  Window& m_window;
  Texture m_texture;
};

}