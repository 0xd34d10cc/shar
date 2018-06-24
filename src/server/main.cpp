#include <thread>
#include <iostream>
#include <chrono>

#include "disable_warnings_push.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "disable_warnings_pop.hpp"

#include "queues/frames_queue.hpp"
#include "queues/packets_queue.hpp"
#include "processors/packet_sender.hpp"
#include "processors/capture_frame_provider.hpp"
#include "processors/frame_display.hpp"
#include "processors/h264encoder.hpp"

namespace sc = SL::Screen_Capture;
namespace pt = boost::property_tree;

int main() {
  pt::ptree root;

  try {
    pt::read_json("config.json", root);
  } catch (const pt::json_parser::json_parser_error&) {
    std::cerr << "config.json corrupted or not found" << std::endl;
    return 0;
  }

  // TODO: make it configurable
  auto        monitor = sc::GetMonitors().front();
  std::size_t width   = root.get<std::size_t>("Capture.width",  static_cast<std::size_t>(monitor.Width));
  std::size_t height  = root.get<std::size_t>("Capture.height", static_cast<std::size_t>(monitor.Height));

  shar::Size  frame_size {height, width};

  using namespace std::chrono_literals;
  const int         fps      = root.get<int>("Capture.fps", 30);
  const auto        interval = 1000ms / fps;
  const std::size_t bitrate  = root.get<int>("Encoder.bitrate", 5000000);

  std::cout << "Capturing " << monitor.Name << " " << width << 'x' << height << std::endl;
  std::cout << "Target bitrate: " << bitrate << ", FPS: " << fps << std::endl;

  shar::FramesQueue  captured_frames;
  shar::PacketsQueue packets_to_send;

  // setup processors pipeline
  shar::CaptureFrameProvider capture {interval,        monitor, captured_frames};
  shar::H264Encoder          encoder {frame_size,      bitrate, fps,
                                      captured_frames, packets_to_send};
  shar::PacketSender         sender  {packets_to_send};

  // start processors
  std::thread capture_thread {[&] {
    capture.run();
  }};

  std::thread encoder_thread {[&] {
    encoder.run();
  }};

  std::thread sender_thread {[&] {
    sender.run();
  }};

  sender.stop();
  encoder.stop();
  capture.stop();

  sender_thread.join();
  encoder_thread.join();
  capture_thread.join();

  return 0;
}