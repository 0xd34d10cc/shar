#include "processors/frame_file_reader.hpp"


using mode = std::ios_base;

namespace shar {

using namespace std::chrono_literals;

FrameFileReader::FrameFileReader(Context context,
                                 FileParams file_params,
                                 Sender<Frame> output)
    : Source(std::move(context), std::move(output))
    , m_file_params(std::move(file_params))
    , m_stream(m_file_params.path, mode::in | mode::binary)
    , m_timer(1000ms / m_file_params.fps) {}

void FrameFileReader::process(FalseInput /*dummy input*/) {
  auto frame = read_frame();
  if (frame.empty()) {
    // EOF
    Processor::stop();
  }

  output().send(std::move(frame));
  m_timer.wait();
  m_timer.restart();
}

shar::Frame FrameFileReader::read_frame() {
  static const std::size_t PIXEL_SIZE = 4;
  const auto& size = m_file_params.frame_size;

  auto total_bytes = static_cast<std::streamsize>(size.width() * size.height() * PIXEL_SIZE);
  auto image       = std::make_unique<std::uint8_t[]>(static_cast<std::size_t>(total_bytes));
  auto read        = std::streamsize {};

  auto* ptr = reinterpret_cast<char*>(image.get());
  while (read != total_bytes) {
    std::streamsize ret = m_stream.readsome(ptr + read, total_bytes - read);
    if (ret == -1 || ret == 0) {
      // EOF?
      m_logger.info("FrameFileReader reached EOF");
      return shar::Frame {};
    }

    read += ret;
  }

  return Frame {std::move(image), size};
}

}