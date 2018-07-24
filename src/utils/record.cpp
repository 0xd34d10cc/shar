#include <chrono>
#include <thread>

#include "queues/frames_queue.hpp"
#include "processors/screen_capture.hpp"
#include "processors/frame_file_writer.hpp"


int main() {
  auto logger = shar::Logger("record_test.log");
  shar::FramesQueue captured_frames;

  using namespace std::chrono_literals;
  std::size_t fps      = 30;
  auto        interval = 1000ms / fps;
  sc::Monitor monitor  = sc::GetMonitors().front();

  shar::ScreenCapture    capture {interval, monitor, logger, captured_frames};
  shar::FrameFileWriter  writer {"example.bgra", logger, captured_frames};

  std::thread capture_thread {[&] {
    capture.run();
  }};

  std::thread writer_thread {[&] {
    writer.run();
  }};

  std::this_thread::sleep_for(5s);

  return 0;
}