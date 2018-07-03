#include <thread>
#include <iostream>
#include <chrono>
#include <condition_variable>
#include <csignal>

#include "disable_warnings_push.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "disable_warnings_pop.hpp"

#include "queues/frames_queue.hpp"
#include "queues/packets_queue.hpp"
#include "processors/packet_sender.hpp"
#include "processors/screen_capture.hpp"
#include "processors/frame_display.hpp"
#include "processors/h264encoder.hpp"


static std::mutex              mutex;
static std::condition_variable signal_to_exit;
static std::atomic<bool>       is_running = false;

namespace sc = SL::Screen_Capture;
namespace pt = boost::property_tree;
namespace ip = boost::asio::ip;

static void signal_handler(int /*signum*/) {
  std::lock_guard lock(mutex);
  is_running = false;
  signal_to_exit.notify_all();
}

int main() {
  // setup signal handler
  if (signal(SIGINT, signal_handler) == SIG_ERR) {
    std::cerr << "Failed to setup signal handler" << std::endl;
  }

  pt::ptree root;

  try {
    pt::read_json("config.json", root);
  } catch (const pt::json_parser::json_parser_error&) {
    std::cerr << "config.json corrupted or not found" << std::endl;
    return 0;
  }

  // TODO: make it configurable
  auto        monitor = sc::GetMonitors().front();
  std::size_t width   = root.get<std::size_t>("width", static_cast<std::size_t>(monitor.Width));
  std::size_t height  = root.get<std::size_t>("height", static_cast<std::size_t>(monitor.Height));

  shar::Size frame_size {height, width};

  using namespace std::chrono_literals;
  const std::size_t fps      = root.get<std::size_t>("fps", 30);
  const auto        interval = 1000ms / fps;
  const std::size_t bitrate  = root.get<std::size_t>("bitrate", 5000000);
  const std::string conf_ip  = root.get<std::string>("host", "127.0.0.1");
  ip::address       host;

  {
    boost::system::error_code err;
    host = ip::address::from_string(conf_ip, err);
    if (err != boost::system::errc::success) {
      std::cerr << "Invalid ip" << std::endl;
      return 0;
    }
  }

  std::cout << "Capturing " << monitor.Name << " " << width << 'x' << height << std::endl;
  std::cout << "Target bitrate: " << bitrate << ", FPS: " << fps << ", ip: " << conf_ip << std::endl;

  shar::FramesQueue  captured_frames;
  shar::PacketsQueue packets_to_send;

  // setup processors pipeline
  shar::ScreenCapture capture {interval, monitor, captured_frames};
  shar::H264Encoder   encoder {frame_size, bitrate, fps,
                               captured_frames, packets_to_send};
  shar::PacketSender  sender {packets_to_send, host};

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

  std::unique_lock lock(mutex);
  is_running = true;
  while (is_running) {
    signal_to_exit.wait(lock);
  }

  std::cerr << "Stopping all processors..." << std::endl;

  sender.stop();
  encoder.stop();
  capture.stop();

  sender_thread.join();
  encoder_thread.join();
  capture_thread.join();

  return 0;
}