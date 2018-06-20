#pragma once

#include <fstream>
#include <string>

#include "primitives/size.hpp"
#include "primitives/image.hpp"
#include "queues/frames_queue.hpp"
#include "processors/processor.hpp"


namespace shar {

class FrameFileWriter : public Processor {
public:
  FrameFileWriter(const std::string& path, FramesQueue& input);
  void run();

private:
  bool write_frame(const shar::Image&);

  FramesQueue& m_input_frames;
  std::ofstream m_stream;
};

}