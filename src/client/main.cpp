#include <thread>

#include "options.hpp"
#include "logger.hpp"
#include "window.hpp"
#include "runner.hpp"
#include "metrics.hpp"
#include "metrics_reporter.hpp"
#include "channels/bounded.hpp"
#include "channels/sink.hpp"
#include "processors/packet_receiver.hpp"
#include "processors/frame_display.hpp"
#include "processors/h264decoder.hpp"


int main(int argc, const char* argv[]) {
  auto       logger = shar::Logger("client.log");
  const auto opts   = shar::Options::from_args(argc, argv);
  auto metrics = std::make_shared<shar::Metrics>(20, logger);

  logger.info("Starting with Host: {}, Screen {}x{}",
              opts.ip.to_string(), opts.width, opts.height);

  const shar::Size frame_size {opts.height, opts.width};
  shar::Window     window {frame_size, logger};;

  auto[packets_sender, packets_receiver] = shar::channel::bounded<shar::Packet>(120);
  auto[frames_sender, frames_receiver] = shar::channel::bounded<shar::Image>(120);

  using FrameSink = shar::channel::Sink<shar::Image>;
  using Display = shar::FrameDisplay<FrameSink>;

  auto receiver = std::make_shared<shar::PacketReceiver>(
      opts.ip,
      logger,
      metrics,
      std::move(packets_sender)
  );

  auto decoder = std::make_shared<shar::H264Decoder>(
      logger,
      metrics,
      std::move(packets_receiver),
      std::move(frames_sender));

  Display display {
      window,
      logger,
      metrics,
      std::move(frames_receiver),
      FrameSink {}
  };

  shar::Runner receiver_runner{std::move(receiver)};
  shar::Runner decoder_runner{std::move(decoder)};
  shar::MetricsReporter reporter{metrics, 10};
  reporter.start();

  // run gui thread
  display.run();
  decoder_runner.stop();
  reporter.stop();

  return EXIT_SUCCESS;
}