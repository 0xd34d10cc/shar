#include <iostream>

#include "processors/frame_file_reader.hpp"


using mode = std::ios_base;

namespace shar {

using namespace std::chrono_literals;

FrameFileReader::FrameFileReader(FileParams file_params,
                                 FramesQueue& output)
    : Processor("FrameFileReader")
    , m_output_frames(output)
    , m_file_params(std::move(file_params))
    , m_stream(m_file_params.path, mode::in | mode::binary)
    , m_timer(1000ms / m_file_params.fps) {}

void FrameFileReader::run() {
  Processor::start();

  while (is_running()) {
    auto frame = read_frame();
    if (frame.empty()) {
      // EOF
      Processor::stop();
      break;
    }

    m_output_frames.push(std::move(frame));
    m_timer.wait();
    m_timer.restart();
  }
}

shar::Image FrameFileReader::read_frame() {
  static const std::size_t PIXEL_SIZE = 4;
  const auto& size = m_file_params.frame_size;

  auto total_bytes = static_cast<std::streamsize>(size.width() * size.height() * PIXEL_SIZE);
  auto image       = std::make_unique<std::uint8_t[]>(static_cast<std::size_t>(total_bytes));
  auto read        = std::streamsize {0};

  auto* ptr = reinterpret_cast<char*>(image.get());
  while (read != total_bytes) {
    std::streamsize ret = m_stream.readsome(ptr + read, total_bytes - read);
    if (ret == -1 || ret == 0) {
      // EOF?
      std::cout << "FrameFileReader reached EOF" << std::endl;
      return shar::Image {};
    }

    read += ret;
  }

  return Image {std::move(image), size};
}

}