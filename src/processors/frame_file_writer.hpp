#pragma once

#include <fstream>
#include <string>

#include "primitives/size.hpp"
#include "primitives/frame.hpp"
#include "channels/bounded.hpp"
#include "processors/processor.hpp"
#include "processors/sink.hpp"


namespace shar {

using FramesReceiver = channel::Receiver<Frame>;

class FrameFileWriter : public Sink<FrameFileWriter, FramesReceiver> {
public:
  using Base = Sink<FrameFileWriter, FramesReceiver>;
  using Context = typename Base::Context;

  FrameFileWriter(Context context, const std::string& path, FramesReceiver input);
  void process(shar::Frame frame);
  void teardown();

private:
  bool write_frame(const shar::Frame&);
  std::ofstream m_stream;
};

}