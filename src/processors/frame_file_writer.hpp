#pragma once

#include <fstream>
#include <string>

#include "primitives/size.hpp"
#include "primitives/image.hpp"
#include "queues/null_queue.hpp"
#include "queues/frames_queue.hpp"
#include "processors/processor.hpp"
#include "processors/sink.hpp"


namespace shar {

class FrameFileWriter : public Sink<FrameFileWriter, FramesQueue> {
public:
  FrameFileWriter(const std::string& path, FramesQueue& input);
  void process(shar::Image* frame);
  void teardown();

private:
  bool write_frame(const shar::Image&);
  std::ofstream m_stream;
};

}