#include <thread>

#include "window.hpp"
#include "primitives/image.hpp"
#include "queues/null_queue.hpp"
#include "queues/frames_queue.hpp"
#include "processors/frame_file_reader.hpp"
#include "processors/frame_display.hpp"


int main() {
  shar::Size   size {1080, 1920};
  shar::Window window {size};

  shar::FileParams  params {"example.bgra", size, 30 /* fps */};
  shar::FramesQueue frames_to_display;
  using FrameSink = shar::NullQueue<shar::Image>;
  FrameSink                     frames_sink;
  shar::FrameFileReader         reader {params, frames_to_display};
  shar::FrameDisplay<FrameSink> display {frames_to_display, frames_sink};

  std::thread reader_thread {[&] {
    reader.run();
  }};

  display.run(window);

  reader.stop();
  display.stop();

  reader_thread.join();

  return 0;
}