#include "processors/frame_file_writer.hpp"


using mode = std::ios_base;

namespace shar {

FrameFileWriter::FrameFileWriter(Context context, const std::string& path, Receiver<Frame> input)
    : Sink(std::move(context), std::move(input))
    , m_stream(path, mode::out | mode::binary) {}

void FrameFileWriter::process(Frame frame) {
  if (!write_frame(frame)) {
    // io error
    Processor::stop();
  }
}

void FrameFileWriter::teardown() {
  m_stream.flush();
  m_stream.close();
}

bool FrameFileWriter::write_frame(const shar::Frame& frame) {
  static const std::size_t PIXEL_SIZE = 4;

  auto total_bytes = static_cast<std::streamsize>(frame.total_pixels() * PIXEL_SIZE);
  auto* ptr = reinterpret_cast<const char*>(frame.bytes());
  m_stream.write(ptr, total_bytes);
  // TODO: handle m_stream.bad()
  return true;
}

}