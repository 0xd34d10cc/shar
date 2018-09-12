#include <thread>
#include <chrono>

#include "runner.hpp"
#include "window.hpp"
#include "channels/bounded.hpp"
#include "channels/sink.hpp"
#include "processors/packet_sender.hpp"
#include "processors/packet_receiver.hpp"
#include "processors/screen_capture.hpp"
#include "processors/display.hpp"
#include "processors/h264encoder.hpp"
#include "processors/h264decoder.hpp"


namespace sc = SL::Screen_Capture;

// "client":
//    PacketReceiver[Server => Packets]
//      -> H264Decoder[Packets => Frames]
//        -> Display[Frames => NULL]

// "server":
//    ScreenCapture[WindowManager => Frames]
//      -> Display[Frames => Frames]
//        -> H264Encoder[Frames => Packets]
//           -> PacketsSender

int main() {
  auto        logger  = shar::Logger("pipeline_test.log");
  auto metrics = std::make_shared<shar::Metrics>(20, logger);
  auto context = shar::ProcessorContext {
    "",
    logger,
    metrics
  };

  auto        monitor = sc::GetMonitors().front();
  std::size_t width   = static_cast<std::size_t>(monitor.Width);
  std::size_t height  = static_cast<std::size_t>(monitor.Height);
  logger.info("Capturing {} {}x{}", monitor.Name, width, height);

  shar::Size   frame_size {height, width};
  shar::Window window {frame_size, logger};;

  using namespace std::chrono_literals;
  const std::size_t fps      = 30;
  const auto        interval = 1000ms / fps;
  shar::IpAddress   ip       = boost::asio::ip::address::from_string("127.0.0.1");

  auto[captured_frames_sender, captured_frames_receiver] =
    shar::channel::bounded<shar::Frame>(120);

  auto[encoded_packets_sender, encoded_packets_receiver] =
    shar::channel::bounded<shar::Packet>(120);

  auto[received_packets_sender, received_packets_receiver] =
    shar::channel::bounded<shar::Packet>(120);

  auto[decoded_frames_sender, decoded_frames_receiver] =
    shar::channel::bounded<shar::Frame>(120);

  const auto config = shar::Config::make_default();

  // setup processors pipeline
  auto capture  = std::make_shared<shar::ScreenCapture>(
      context.with_name("Capture"),
      interval,
      monitor,
      std::move(captured_frames_sender)
  );
  auto encoder  = std::make_shared<shar::H264Encoder>(
      context.with_name("Encoder"),
      frame_size,
      fps,
      config.get_subconfig("encoder"),
      std::move(captured_frames_receiver),
      std::move(encoded_packets_sender)
  );
  auto sender   = std::make_shared<shar::PacketSender>(
      context.with_name("PacketSender"),
      ip,
      std::move(encoded_packets_receiver)
  );
  auto receiver = std::make_shared<shar::PacketReceiver>(
      context.with_name("PacketReceiver"),
      ip,
      std::move(received_packets_sender)
  );
  auto decoder  = std::make_shared<shar::H264Decoder>(
      context.with_name("Decoder"),
      std::move(received_packets_receiver),
      std::move(decoded_frames_sender)
  );

  shar::Display display {
      context.with_name("Display"),
      window,
      std::move(decoded_frames_receiver)
  };

  shar::Runner capture_runner {std::move(capture)};
  shar::Runner encoder_runner {std::move(encoder)};
  shar::Runner sender_runner {std::move(sender)};
  shar::Runner receiver_runner {std::move(receiver)};
  shar::Runner decoder_runner {std::move(decoder)};

  // run gui thread
  display.run();

  // stop "client" half of pipeline
  // NOTE: we have to do this because |display| process is not destroyed yet
  //       so frames receiver in |display| is not destroyed also which means
  //       that |decoder| <-> |display| channel is still connected
  decoder_runner.stop();

  // stop "server" half of pipeline
  encoder_runner.stop();

  return EXIT_SUCCESS;
}