#include <fstream>

#include "app.hpp"

#include "common/registry.hpp"
#include "network/network.hpp"
#include "signal_handler.hpp"


namespace shar {

static Context make_context() {
  auto logger = Logger("shar.log"); // TODO: make configurable
  auto config = [&]{
    try {
      std::fstream config_file{"config.json"};
      return Config::parse(config_file);
    }
    catch (const std::exception& e) {
      logger.warning("Unable to read config: {}. Default will be used.", e.what());
      return Config::make_default();
    }
  }();
  auto registry = std::make_shared<shar::metrics::Registry>(logger);

  return Context{ std::move(config), std::move(logger), std::move(registry) };
}

static sc::Monitor select_monitor(const Context& context) {
  auto i = context.m_config->get<std::size_t>("monitor", 0);
  auto monitors = sc::GetMonitors();

  if (i >= monitors.size()) {
    std::string monitors_list;
    for (const auto& monitor : monitors) {
      monitors_list += fmt::format("{} {} {}x{}\n",
        monitor.Index, monitor.Name,
        monitor.Width, monitor.Height);
    }
    context.m_logger.info("Available monitors:\n{}", monitors_list);
    throw std::runtime_error(fmt::format("Selected (#{}) monitor is unavailable", i));
  }

  return monitors[i];
}

static Capture create_capture(Context context, sc::Monitor monitor) {
  using namespace std::chrono_literals;
  const auto fps = context.m_config->get<std::size_t>("fps", 30);
  const auto interval = 1000ms / fps;

  context.m_logger.info("Capturing {} {}x{}",
    monitor.Name, monitor.Width, monitor.Height);
  context.m_logger.info("FPS: {}", fps);

  return Capture{ std::move(context), interval, std::move(monitor) };
}

static Encoder create_encoder(Context context, const sc::Monitor& monitor) {
  const auto fps = context.m_config->get<std::size_t>("fps", 30);

  return Encoder{
    std::move(context),
    Size{
      static_cast<std::size_t>(monitor.Height),
      static_cast<std::size_t>(monitor.Width)
    },
    fps
  };
}

static ExposerPtr create_exposer(const Context& context) {
  const auto host = context.m_config->get<std::string>("metrics", "127.0.0.1:3228");
  return std::make_unique<prometheus::Exposer>(host);
}

static NetworkPtr create_network(Context context) {
  const auto url_str = context.m_config->get<std::string>("url", "tcp://127.0.0.1:1337");
  const auto url = Url::from_string(url_str);

  context.m_logger.info("Streaming to {}", url.to_string());
  return create_module(std::move(context), std::move(url));
}

App::App(int /*argc*/, const char* /*argv*/[])
  : m_context(make_context())
  , m_monitor(select_monitor(m_context))
  , m_capture(create_capture(m_context, m_monitor))
  , m_encoder(create_encoder(m_context, m_monitor))
  , m_network(create_network(m_context))
  // TODO: make optional
  , m_exposer(create_exposer(m_context))
{
  static bool success = SignalHandler::setup();
  if (!success) {
    throw std::runtime_error("Failed to initialize signal handler");
  }

  m_context.m_registry->register_on(*m_exposer);
}

int App::run() {
  auto[frames_tx, frames_rx] = channel<Frame>(120);
  auto[packets_tx, packets_rx] = channel<Packet>(120);

  // NOTE: current capture implementation starts background thread.
  m_capture.run(std::move(frames_tx));

  std::thread encoder_thread{ [this,
                               rx{std::move(frames_rx)},
                               tx{std::move(packets_tx)}] () mutable {
    m_encoder.run(std::move(rx), std::move(tx));
  } };

  std::thread network_thread{ [this,
                               rx{std::move(packets_rx)}]() mutable {
    m_network->run(std::move(rx));
  } };

  shar::SignalHandler::wait_for_sigint();

  m_capture.shutdown();
  m_encoder.shutdown();
  m_network->shutdown();

  encoder_thread.join();
  network_thread.join();

  return EXIT_SUCCESS;
}

}