#include <thread>
#include <iostream>
#include <chrono>
#include <cassert>

#include "window.hpp"
#include "queues/null_queue.hpp"
#include "queues/frames_queue.hpp"
#include "processors/capture_frame_provider.hpp"
#include "processors/frame_display.hpp"


namespace sc = SL::Screen_Capture;

// "client":
//    PacketReceiver[Server => Packets]
//      -> H264Decoder[Packets => Frames]
//        -> FrameDisplay[Frames => NULL]

// "server":
//    CaptureFrameProvider[WindowManager => Frames]
//      -> FrameDisplay[Frames => Frames]
//        -> H264Encoder[Frames => Packets]
//           -> PacketsSender

int main() {
  shar::FramesQueue pipeline;

  // TODO: make it configurable
  auto        monitor = SL::Screen_Capture::GetMonitors().front();
  std::size_t width   = static_cast<std::size_t>(monitor.Width);
  std::size_t height  = static_cast<std::size_t>(monitor.Height);
  std::cout << "Capturing " << monitor.Name << " " << width << 'x' << height << std::endl;

  shar::Window window {shar::Size {height, width}};;

  using namespace std::chrono_literals;
  const int  fps      = 60;
  const auto interval = 1000ms / fps;

  shar::FramesQueue frames;

  using Sink = shar::NullQueue<shar::Image>;
  Sink                       sink;
  shar::CaptureFrameProvider capture {interval, monitor, frames};
  shar::FrameDisplay<Sink>   display {frames, sink};

  // start processors
  std::thread capture_thread {[&] {
    capture.run();
  }};

  // run gui thread
  display.run(window);

  // stop all processors
  display.stop();
  capture.stop();

  capture_thread.join();

  return 0;
}