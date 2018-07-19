#include <thread>
#include <iostream>
#include <chrono>
#include <condition_variable>
#include <csignal>

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
namespace ip = boost::asio::ip;

static void signal_handler(int /*signum*/) {
  std::lock_guard<std::mutex> lock(mutex);
  is_running = false;
  signal_to_exit.notify_all();
}

static int run() {
  // setup signal handler
  if (signal(SIGINT, signal_handler) == SIG_ERR) {
    std::cerr << "Failed to setup signal handler" << std::endl;
  }

  auto config = shar::Config::parse_from_file("config.json");

  // TODO: make it configurable
  auto        monitor = sc::GetMonitors().front();
  std::size_t width   = config.get<std::size_t>("width", static_cast<std::size_t>(monitor.Width));
  std::size_t height  = config.get<std::size_t>("height", static_cast<std::size_t>(monitor.Height));

  shar::Size frame_size {height, width};

  using namespace std::chrono_literals;
  const std::size_t fps      = config.get<std::size_t>("fps", 30);
  const auto        interval = 1000ms / fps;
  const auto        ip_str   = config.get<std::string>("host", "127.0.0.1");
  const ip::address ip       = ip::address::from_string(ip_str);

  std::cout << "Capturing " << monitor.Name << " " << width << 'x' << height << std::endl;
  std::cout << "FPS: " << fps << std::endl;
  std::cout << "IP: " << ip << std::endl;

  const auto encoder_config = config.get_subconfig("encoder");
  std::cout << "Encoder config: " << encoder_config.to_string() << std::endl;

  shar::FramesQueue  captured_frames;
  shar::PacketsQueue packets_to_send;

  // setup processors pipeline
  shar::ScreenCapture capture {interval, monitor, captured_frames};
  shar::H264Encoder   encoder {frame_size, fps, encoder_config,
                               captured_frames, packets_to_send};
  shar::PacketSender  sender {packets_to_send, ip};

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

  // wait for sigint (ctrl+c)
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

  return EXIT_SUCCESS;
}

int main(int /*argc*/, char* /*argv*/[]) {
  try {
    return run();
  } catch (const std::exception& e) {
    std::cerr << "An error occurred: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}