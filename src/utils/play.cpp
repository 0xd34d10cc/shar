#include <cstdlib>

#include "window.hpp"
#include "runner.hpp"
#include "primitives/frame.hpp"
#include "channels/bounded.hpp"
#include "channels/sink.hpp"
#include "processors/frame_file_reader.hpp"
#include "processors/display.hpp"


int main() {
  auto         logger = shar::Logger("play.log");
  shar::Size   size {1080, 1920};
  shar::Window window {size, logger};

  shar::FileParams params {"example.bgra", size, 30 /* fps */};
  auto metrics = std::make_shared<shar::Metrics>(20, logger);

  auto context = shar::ProcessorContext {
    "",
    logger,
    metrics
  };

  auto[frames_sender, frames_receiver] = shar::channel::bounded<shar::Frame>(120);

  auto reader  = std::make_shared<shar::FrameFileReader>(
      context.with_name("FileReader"),
      params,
      std::move(frames_sender)
  );
  auto display = shar::FrameDisplay {
      context.with_name("Display"),
      window,
      std::move(frames_receiver)
  };

  shar::Runner reader_runner {std::move(reader)};
  display.run();
  return EXIT_SUCCESS;
}