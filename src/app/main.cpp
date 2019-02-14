#include <thread>

#include <chrono>

#include "disable_warnings_push.hpp"
#include "prometheus/registry.h"
#include "prometheus/exposer.h"
#include "fmt/format.h"
#include "disable_warnings_pop.hpp"

#include "signal_handler.hpp"
#include "channel.hpp"

#include "network/url.hpp"
#include "encoder/ffmpeg/codec.hpp"
#include "capture/capture.hpp"
#include "encoder/encoder.hpp"
#include "network/network.hpp"
#include "network/consts.hpp"


namespace sc = SL::Screen_Capture;
namespace ip = boost::asio::ip;
using namespace shar;

using NetworkPtr = std::unique_ptr<INetworkModule>;

static sc::Monitor select_monitor(Context& context) {
  auto i = context.m_config->get<std::size_t>("monitor", 0);
  auto monitors = sc::GetMonitors();

  if (i>= monitors.size()) {
    std::string monitorsList;
    for (const auto& monitor : monitors) {
      monitorsList += fmt::format("{} {} {}x{}\n",
                                  monitor.Index, monitor.Name,
                                  monitor.Width, monitor.Height);
    }
    context.m_logger.info("Available monitors:\n" + monitorsList);
    throw std::runtime_error(fmt::format("Selected (#{}) monitor is unavailable", i));
  }

  return monitors[i];
}

static Capture create_capture(Context context, const sc::Monitor& monitor) {
  using namespace std::chrono_literals;
  const auto fps = context.m_config->get<std::size_t>("fps", 30);
  const auto interval = 1000ms / fps;

  context.m_logger.info("Capturing {} {}x{}",
                        monitor.Name, monitor.Width, monitor.Height);
  context.m_logger.info("FPS: {}", fps);

  return Capture{ std::move(context), interval, monitor };
}

static Encoder create_encoder(Context context, const sc::Monitor& monitor) {
  auto width = context.m_config->get<std::size_t>("width", static_cast<std::size_t>(monitor.Width));
  auto height = context.m_config->get<std::size_t>("height", static_cast<std::size_t>(monitor.Height));
  if (width > monitor.Width) {
    context.m_logger.error("Selected width ({}) is greater than monitor width ({}). "
                           "It will be ignored",
                           width, monitor.Width);
    width = static_cast<std::size_t>(monitor.Width);
  }

  if (height > monitor.Height) {
    context.m_logger.error("Selected height ({}) is greater than monitor height ({}). "
                           "It will be ignored",
                           height, monitor.Height);
    height = static_cast<std::size_t>(monitor.Height);
  }

  shar::Size frame_size{ height, width };
  const auto fps = context.m_config->get<std::size_t>("fps", 30);
  return Encoder{ std::move(context), frame_size, fps };
}

static NetworkPtr create_network(Context context) {
  const auto url_str = context.m_config->get<std::string>("url", "tcp://127.0.0.1:4444");
  const auto url = Url::from_string(url_str);

  context.m_logger.info("Streaming to {}", url.to_string());
  return create_module(std::move(context), std::move(url));
}

static prometheus::Exposer create_exposer(const Context& context) {
  const auto host = context.m_config->get<std::string>("metrics", "127.0.0.1:3228");

  return prometheus::Exposer(host);
}
static Context make_context() {
  auto config = shar::Config::parse_from_file("config.json");
  auto logger = Logger("server.log");
  auto metrics = std::make_shared<shar::Metrics>(logger);

  return Context{ config, logger, metrics };
}

static int run() {
  auto context = make_context();
  auto monitor = select_monitor(context);
  auto exposer = create_exposer(context);
  auto capture = create_capture(context, monitor);
  auto encoder = create_encoder(context, monitor);
  auto network = create_network(context);
  context.m_metrics->register_on(exposer);

  if (!shar::SignalHandler::setup()) {
    context.m_logger.error("Failed to setup signal handler");
    return EXIT_FAILURE;
  }

  auto[frames_tx, frames_rx] = channel<Frame>(120);
  auto[packets_tx, packets_rx] = channel<Packet>(120);

  capture.run(std::move(frames_tx));

  std::thread encoder_thread{ [&encoder,
                               rx{std::move(frames_rx)},
                               tx{std::move(packets_tx)}] () mutable {
    encoder.run(std::move(rx), std::move(tx));
  }};

  std::thread network_thread{ [&network,
                               rx{std::move(packets_rx)}]() mutable {
    network->run(std::move(rx));
  }};

  shar::SignalHandler::wait_for_sigint();

  capture.shutdown();
  encoder.shutdown();
  network->shutdown();

  encoder_thread.join();
  network_thread.join();

  return EXIT_SUCCESS;
}

int main(int /*argc*/, char* /*argv*/[]) {
  try {
    return run();
  }
  catch (const std::exception& e) {
    std::cerr << "An error occurred: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}