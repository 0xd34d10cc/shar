#pragma once

#include "window.hpp"
#include "texture.hpp"
#include "primitives/point.hpp"
#include "primitives/size.hpp"
#include "processors/processor.hpp"
#include "queues/frames_queue.hpp"


namespace shar {

// Note: OutputQueue should be frames queue
template<typename OutputQueue>
class FrameDisplay : public Processor<FrameDisplay<OutputQueue>, FramesQueue, OutputQueue> {
public:
  using BaseProcessor = Processor<FrameDisplay<OutputQueue>, FramesQueue, OutputQueue>;

  // NOTE: |window| should be initialized in same
  // thread from which run() will be called
  FrameDisplay(Window& window, Logger logger, FramesQueue& input, OutputQueue& output)
      : BaseProcessor("FrameDisplay", logger, input, output)
      , m_window(window)
      , m_texture(window.size()) {}

  void setup() {
    m_texture.bind();
  }

  void teardown() {
    m_texture.unbind();
  }

  // FIXME: this processor should not (?) depend on window
  void process(Image* frame) {
//    std::cerr << "Displaying frame: "
//              << frame->width() << 'x' << frame->height() << std::endl;

    m_texture.update(Point::origin(),
                     frame->size(),
                     frame->bytes());
    m_window.draw_texture(m_texture);
    m_window.swap_buffers();
    m_window.poll_events();

    if (m_window.should_close()) {
      BaseProcessor::stop();
      return;
    }

    BaseProcessor::output().push(std::move(*frame));
  }

private:
  Window& m_window;
  Texture m_texture;
};

}