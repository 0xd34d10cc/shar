#pragma once

#include <fstream>
#include <string>
#include <cstddef>

#include "primitives/size.hpp"
#include "primitives/timer.hpp"
#include "primitives/image.hpp"
#include "channels/bounded.hpp"
#include "processors/source.hpp"


namespace shar {

struct FileParams {
  std::string path;
  Size        frame_size;
  std::size_t fps;
};

using FramesSender = channel::Sender<Image>;

class FrameFileReader : public Source<FrameFileReader, FramesSender> {
public:
  using Base = Source<FrameFileReader, FramesSender>;
  using Context = typename Base::Context;

  FrameFileReader(Context context, FileParams params, FramesSender output);

  FrameFileReader(const FrameFileReader&) = delete;
  FrameFileReader(FrameFileReader&& other) = default;

  void process(FalseInput /* dummy input */);

private:
  shar::Image read_frame();

  FileParams    m_file_params;
  std::ifstream m_stream;
  shar::Timer   m_timer;
};

}