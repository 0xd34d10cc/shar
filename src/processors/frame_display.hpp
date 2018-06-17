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
class FrameDisplay : public Processor {
public:
  FrameDisplay(FramesQueue& input, OutputQueue& output)
      : m_frames(input)
      , m_frames_consumer(output) {}

  // FIXME: this processor should not (?) depend on window
  void run(Window& window) {
    Processor::start();

    shar::Texture texture {window.size()};
    texture.bind();

    while (is_running() && !window.should_close()) {
      if (!m_frames.empty()) {
        do {
          shar::Image* frame = m_frames.get_next();
          texture.update(Point::origin(),
                         frame->size(),
                         frame->bytes());

          m_frames_consumer.push(std::move(*frame));
          m_frames.consume(1);
        } while (!m_frames.empty());

        window.draw_texture(texture);
        window.swap_buffers();
      }

      window.poll_events();
      m_frames.wait();
    }

    if (is_running()) {
      Processor::stop();
    }

  }

private:
  FramesQueue& m_frames;
  OutputQueue& m_frames_consumer;

};

}