#include <thread>

#include <chrono>

#include "signal_handler.hpp"
#include "metrics_reporter.hpp"
#include "channel.hpp"

#include "context.hpp"
#include "capture/capture.hpp"
#include "encoder/encoder.hpp"
#include "network/network.hpp"
#include "network/consts.hpp"


namespace sc = SL::Screen_Capture;
namespace ip = boost::asio::ip;
using namespace shar;

using CapturePtr = std::shared_ptr<Capture>;
using EncoderPtr = std::shared_ptr<Encoder>;
using NetworkPtr = std::shared_ptr<Network>;

static CapturePtr create_capture(Context context) {
  auto monitor = sc::GetMonitors().front();

  using namespace std::chrono_literals;
  const auto fps      = context.m_config->get<std::size_t>("fps", 30);
  const auto interval = 1000ms / fps;

  context.m_logger.info("Capturing {} {}x{}",
                      monitor.Name, monitor.Width, monitor.Height);
  context.m_logger.info("FPS: {}", fps);

  return std::make_shared<Capture>(std::move(context), 
                                   std::move(interval), 
                                   std::move(monitor));  
}

static EncoderPtr create_encoder(Context context) {
  auto monitor = sc::GetMonitors().front();
  auto width   = context.m_config->get<std::size_t>("width", static_cast<std::size_t>(monitor.Width));
  auto height  = context.m_config->get<std::size_t>("height", static_cast<std::size_t>(monitor.Height));
  shar::Size frame_size {height, width};

  const auto fps = context.m_config->get<std::size_t>("fps", 30);

  return std::make_shared<Encoder>(std::move(context), frame_size, fps);
}

static NetworkPtr create_network(Context context) {
  const auto ip_str = context.m_config->get<std::string>("host", "127.0.0.1");
  const ip::address  ip = ip::address::from_string(ip_str);
  const std::uint16_t port = context.m_config->get<std::uint16_t>("port", shar::SERVER_DEFAULT_PORT);

  context.m_logger.info("IP: {}", ip_str);
  return std::make_shared<Network>(std::move(context), 
                                   std::move(ip), 
                                   port);
}

static Context make_context() {
  auto config = shar::Config::parse_from_file("config.json");
  auto logger  = Logger("server.log");
  auto metrics = std::make_shared<shar::Metrics>(20, logger);

  return Context{config, logger, metrics};
}

static int run() {
  auto context = make_context();
  auto capture = create_capture(context);
  auto encoder = create_encoder(context);
  auto network = create_network(context);

  if (!shar::SignalHandler::setup()) {
    context.m_logger.error("Failed to setup signal handler");
    return EXIT_FAILURE;
  }

  auto [frames_tx, frames_rx]   = channel<Frame>(120);
  auto [packets_tx, packets_rx] = channel<Packet>(120);

  capture->run(std::move(frames_tx));

  std::thread encoder_thread {[encoder, 
                               rx{std::move(frames_rx)},
                               tx{std::move(packets_tx)}] () mutable {
    encoder->run(std::move(rx), std::move(tx));
  }};

  std::thread network_thread {[network,
                               rx{std::move(packets_rx)}] () mutable {
    network->run(std::move(rx));
  }};

  shar::MetricsReporter reporter {context.m_metrics, 10};
  reporter.start();

  shar::SignalHandler::wait_for_sigint();
  
  capture->shutdown();
  encoder->shutdown();
  network->shutdown();

  encoder_thread.join();
  network_thread.join();

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