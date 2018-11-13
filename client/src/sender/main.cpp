#include <thread>

#include <chrono>

#include "logger.hpp"
#include "runner.hpp"
#include "signal_handler.hpp"
#include "network/forwarding.hpp"
#include "metrics.hpp"
#include "metrics_reporter.hpp"
#include "channels/bounded.hpp"
#include "network/consts.hpp"
#include "processors/packet_forwarder.hpp"
#include "processors/screen_capture.hpp"
#include "processors/h264encoder.hpp"


namespace sc = SL::Screen_Capture;
namespace ip = boost::asio::ip;

static int run() {
  auto logger  = shar::Logger("server.log");
  auto metrics = std::make_shared<shar::Metrics>(20, logger);
  auto context = shar::ProcessorContext {
      "",
      logger,
      metrics
  };

  if (!shar::SignalHandler::setup()) {
    logger.error("Failed to setup signal handler");
    return EXIT_FAILURE;
  }

  auto config = shar::Config::parse_from_file("config.json");

  // setup port forwarding
  shar::forward_port(
      shar::SERVER_DEFAULT_PORT /* local */,
      shar::SERVER_DEFAULT_PORT /* remote */,
      logger
  );

  // TODO: allow monitor selection for capture
  auto        monitor = sc::GetMonitors().front();
  std::size_t width   = config.get<std::size_t>("width", static_cast<std::size_t>(monitor.Width));
  std::size_t height  = config.get<std::size_t>("height", static_cast<std::size_t>(monitor.Height));

  shar::Size frame_size {height, width};

  using namespace std::chrono_literals;
  const std::size_t fps      = config.get<std::size_t>("fps", 30);
  const auto        interval = 1000ms / fps;
  const auto        ip_str   = config.get<std::string>("host", "127.0.0.1");
  const ip::address ip       = ip::address::from_string(ip_str);
  const std::uint16_t port = config.get<std::uint16_t>("port", 1337);

  logger.info("Capturing {} {}x{}", monitor.Name, width, height);
  logger.info("FPS: {}", fps);
  logger.info("IP: {}", ip_str);

  const auto encoder_config = config.get_subconfig("encoder");
  logger.info("Encoder config: {}", encoder_config.to_string());

  auto[captured_frames_sender, captured_frames_receiver] =
  shar::channel::bounded<shar::Frame>(120);
  auto[encoded_packets_sender, encoded_packets_receiver] =
  shar::channel::bounded<shar::Packet>(120);

  // setup processors pipeline
  auto capture = std::make_shared<shar::ScreenCapture>(
      context.with_name("Capture"),
      interval,
      monitor,
      std::move(captured_frames_sender)
  );
  auto encoder = std::make_shared<shar::H264Encoder>(
      context.with_name("Encoder"),
      frame_size,
      fps,
      encoder_config,
      std::move(captured_frames_receiver),
      std::move(encoded_packets_sender)
  );
  auto sender  = std::make_shared<shar::PacketForwarder>(
      context.with_name("PacketForwarder"),
      ip,
      port,
      std::move(encoded_packets_receiver)
  );

  shar::Runner capture_runner {std::move(capture)};
  shar::Runner encoder_runner {std::move(encoder)};
  shar::Runner sender_runner {std::move(sender)};

  shar::MetricsReporter reporter {metrics, 10};
  reporter.start();

  shar::SignalHandler::wait_for_sigint();
  sender_runner.stop();

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