#include <thread>

#include <chrono>
#include <condition_variable>
#include <csignal>

#include "logger.hpp"
#include "queues/frames_queue.hpp"
#include "queues/packets_queue.hpp"
#include "processors/packet_sender.hpp"
#include "processors/screen_capture.hpp"
#include "processors/frame_display.hpp"
#include "processors/h264encoder.hpp"

static const sighandler_t SIGNAL_ERROR = static_cast<sighandler_t>(-1);

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
  auto logger = shar::Logger("server.log");

  // setup signal handler
  if (signal(SIGINT, signal_handler) == SIGNAL_ERROR) {
    logger.error("Failed to setup signal handler");
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

  logger.info("Capturing {} {}x{}", monitor.Name, width, height);
  logger.info("FPS: {}", fps);
  logger.info("IP: {}", ip_str);

  const auto encoder_config = config.get_subconfig("encoder");
  logger.info("Encoder config: {}", encoder_config.to_string());

  shar::FramesQueue  captured_frames;
  shar::PacketsQueue packets_to_send;

  // setup processors pipeline
  shar::ScreenCapture capture {interval, monitor, logger, captured_frames};
  shar::H264Encoder   encoder {frame_size, fps, encoder_config,
                               logger, captured_frames, packets_to_send};
  shar::PacketSender  sender {packets_to_send, ip, logger};

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
  std::unique_lock<std::mutex> lock(mutex);
  is_running = true;
  while (is_running) {
    signal_to_exit.wait(lock);
  }

  logger.info("Stopping all processors...");

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