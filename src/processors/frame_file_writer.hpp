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
  using Base = Sink<FrameFileWriter, FramesReceiver>;
  using Context = typename Base::Context;

  FrameFileWriter(Context context, const std::string& path, FramesReceiver input);
  void process(shar::Image frame);
  void teardown();

private:
  bool write_frame(const shar::Image&);
  std::ofstream m_stream;
};

}