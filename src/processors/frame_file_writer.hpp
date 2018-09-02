#pragma once

#include <fstream>
#include <string>

#include "primitives/size.hpp"
#include "primitives/image.hpp"
#include "channels/bounded.hpp"
#include "processors/processor.hpp"
#include "processors/sink.hpp"


namespace shar {

using FramesReceiver = channel::Receiver<Image>;

class FrameFileWriter : public Sink<FrameFileWriter, FramesReceiver> {
public:
  FrameFileWriter(const std::string& path, Logger logger, FramesReceiver input);
  void process(shar::Image frame);
  void teardown();

private:
  bool write_frame(const shar::Image&);
  std::ofstream m_stream;
};

}