#include <fstream>

#include "broadcast.hpp"
#include "metrics/registry.hpp"
#include "network/sender_factory.hpp"


namespace shar {

static sc::Monitor select_monitor(const Context& context) {
  auto i = context.m_config->monitor;
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
  const auto fps = context.m_config->fps;
  const auto interval = 1000ms / fps;

  context.m_logger.info("Capturing {} {}x{}",
    monitor.Name, monitor.Width, monitor.Height);
  return Capture{ std::move(context), interval, std::move(monitor) };
}

static codec::Encoder create_encoder(Context context, const sc::Monitor& monitor) {
  const auto fps = context.m_config->fps;

  return codec::Encoder{
    std::move(context),
    Size{
      static_cast<std::size_t>(monitor.Height),
      static_cast<std::size_t>(monitor.Width)
    },
    fps
  };
}

static SenderPtr create_network(Context context) {
  const auto url_str = context.m_config->url;
  const auto url = Url::from_string(url_str);
  return create_sender(std::move(context), std::move(url));
}

Broadcast::Broadcast(Context context)
  : m_context(context)
  , m_monitor(select_monitor(m_context))
  , m_capture(create_capture(m_context, m_monitor))
  , m_encoder(create_encoder(m_context, m_monitor))
  , m_network(create_network(m_context))
{}

Receiver<codec::ffmpeg::Frame> Broadcast::start() {
  auto[display_frames_tx, display_frames_rx] = channel<codec::ffmpeg::Frame>(30);
  auto[frames_tx, frames_rx] = channel<codec::ffmpeg::Frame>(30);
  auto[packets_tx, packets_rx] = channel<codec::ffmpeg::Unit>(30);

  // NOTE: current capture implementation starts background thread.
  m_capture.run(std::move(frames_tx), std::move(display_frames_tx));

  m_encoder_thread = std::thread{ [this,
                                   rx{std::move(frames_rx)},
                                   tx{std::move(packets_tx)}] () mutable {
    m_encoder.run(std::move(rx), std::move(tx));
  } };

  m_network_thread = std::thread{ [this,
                                   rx{std::move(packets_rx)}]() mutable {
    m_network->run(std::move(rx));
  } };

  return std::move(display_frames_rx);
}

void Broadcast::stop() {
  m_capture.shutdown();
  m_encoder.shutdown();
  m_network->shutdown();

  m_encoder_thread.join();
  m_network_thread.join();
}

}