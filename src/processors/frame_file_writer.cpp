#include "processors/frame_file_writer.hpp"


using mode = std::ios_base;

namespace shar {

FrameFileWriter::FrameFileWriter(const std::string& path, FramesQueue& input)
    : Processor("FrameFileWriter")
    , m_input_frames(input)
    , m_stream(path, mode::out | mode::binary) {}

void FrameFileWriter::run() {
  Processor::start();
  while (is_running()) {
    m_input_frames.wait();

    if (!m_input_frames.empty()) {
      do {
        auto* frame = m_input_frames.get_next();

        if (!write_frame(*frame)) {
          // io error
          Processor::stop();
          break;
        }

        m_input_frames.consume(1);
      } while (!m_input_frames.empty());
    }
  }

  m_stream.flush();
  m_stream.close();
}

bool FrameFileWriter::write_frame(const shar::Image& frame) {
  static const std::size_t PIXEL_SIZE = 4;

  auto total_bytes = static_cast<std::streamsize>(frame.total_pixels() * PIXEL_SIZE);
  auto* ptr = reinterpret_cast<const char*>(frame.bytes());
  m_stream.write(ptr, total_bytes);
  // TODO: handle m_stream.bad()
  return true;
}

}