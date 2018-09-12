#pragma once

#include <fstream>
#include <string>

#include "primitives/size.hpp"
#include "primitives/frame.hpp"
#include "channels/bounded.hpp"
#include "processors/processor.hpp"
#include "processors/sink.hpp"


namespace shar {

class FrameFileWriter : public Sink<FrameFileWriter, Receiver<Frame>> {
public:
  using Base = Sink<FrameFileWriter, Receiver<Frame>>;
  using Context = typename Base::Context;

  FrameFileWriter(Context context, const std::string& path, Receiver<Frame> input);
  void process(shar::Frame frame);
  void teardown();

private:
  bool write_frame(const shar::Frame&);
  std::ofstream m_stream;
};

}