#pragma once

#include "window.hpp"
#include "texture.hpp"
#include "primitives/point.hpp"
#include "primitives/size.hpp"
#include "primitives/frame.hpp"
#include "processors/sink.hpp"
#include "channels/bounded.hpp"


namespace shar {

using FramesReceiver = channel::Receiver<Frame>;

// TODO: remove generic parameter
class FrameDisplay : public Sink<FrameDisplay, FramesReceiver> {
public:
  using Base = Sink<FrameDisplay, FramesReceiver>;
  using Context = typename Base::Context;

  // NOTE: |window| should be initialized in same
  // thread from which run() will be called
  FrameDisplay(Context context, Window& window, FramesReceiver input);

  void setup() {
    m_fps_metric = m_metrics->add("Display\tfps", Metrics::Format::Count);
    m_texture.bind();
  }

  void teardown() {
    m_texture.unbind();
    m_metrics->remove(m_fps_metric);
  }

  void process(Frame frame) {
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

private:
  MetricId m_fps_metric;
  Window& m_window;
  Texture m_texture;
};

}