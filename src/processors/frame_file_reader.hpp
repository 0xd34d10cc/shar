#pragma once

#include <fstream>
#include <string>
#include <cstddef>

#include "primitives/size.hpp"
#include "primitives/timer.hpp"
#include "queues/frames_queue.hpp"
#include "processors/processor.hpp"


namespace shar {

struct FileParams {
  std::string path;
  Size        frame_size;
  std::size_t fps;
};

class FrameFileReader : public Processor {
public:
  FrameFileReader(FileParams params, FramesQueue& output);

  void run();
  shar::Image read_frame();

private:
  FramesQueue& m_output_frames;
  FileParams    m_file_params;
  std::ifstream m_stream;
  shar::Timer   m_timer;
};

}