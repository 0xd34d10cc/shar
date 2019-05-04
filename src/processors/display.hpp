#pragma once

#include "window.hpp"
#include "texture.hpp"
#include "primitives/point.hpp"
#include "primitives/size.hpp"
#include "primitives/frame.hpp"
#include "processors/sink.hpp"
#include "channels/bounded.hpp"


namespace shar {

class Display : public Sink<Display, Receiver<Frame>> {
public:
  using Base = Sink<Display, Receiver<Frame>>;
  using Context = typename Base::Context;

  // NOTE: |window| should be initialized in same
  // thread from which run() will be called
  Display(Context context, Window& window, Receiver<Frame> input);

  void setup();
  void teardown();

  void process(Frame frame);

private:
  MetricId m_fps_metric;
  Window& m_window;
  Texture m_texture;
};

}