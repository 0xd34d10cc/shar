#pragma once

#include <fstream>
#include <string>
#include <cstddef>

#include "primitives/size.hpp"
#include "primitives/timer.hpp"
#include "queues/frames_queue.hpp"
#include "processors/source.hpp"


namespace shar {

struct FileParams {
  std::string path;
  Size        frame_size;
  std::size_t fps;
};

class FrameFileReader : public Source<FrameFileReader, FramesQueue> {
public:
  FrameFileReader(FileParams params, FramesQueue& output);

  void process(Void* /* dummy input */);
private:
  shar::Image read_frame();

  FileParams    m_file_params;
  std::ifstream m_stream;
  shar::Timer   m_timer;
};

}