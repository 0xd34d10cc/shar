#pragma once

#include "window.hpp"
#include "texture.hpp"
#include "primitives/point.hpp"
#include "primitives/size.hpp"
#include "primitives/image.hpp"
#include "processors/processor.hpp"
#include "channels/bounded.hpp"


namespace shar {

using FramesReceiver = channel::Receiver<Image>;

// Note: OutputQueue should be frames sender
template<typename Output>
class FrameDisplay : public Processor<FrameDisplay<Output>, FramesReceiver, Output> {
public:
  using BaseProcessor = Processor<FrameDisplay<Output>, FramesReceiver, Output>;

  // NOTE: |window| should be initialized in same
  // thread from which run() will be called
  FrameDisplay(Window& window, Logger logger, FramesReceiver input, Output output)
      : BaseProcessor("FrameDisplay", logger, std::move(input), std::move(output))
      , m_window(window)
      , m_texture(window.size()) {}

  void setup() {
    m_texture.bind();
  }

  void teardown() {
    m_texture.unbind();
  }

  void process(Image frame) {
    m_texture.update(Point::origin(),
                     frame.size(),
                     frame.bytes());
    m_window.draw_texture(m_texture);
    m_window.swap_buffers();
    m_window.poll_events();

    if (m_window.should_close()) {
      BaseProcessor::stop();
      return;
    }

    BaseProcessor::output().send(std::move(frame));
  }

private:
  Window& m_window;
  Texture m_texture;
};

}