#include <thread>

#include "options.hpp"
#include "logger.hpp"
#include "window.hpp"
#include "queues/null_queue.hpp"
#include "queues/frames_queue.hpp"
#include "queues/packets_queue.hpp"
#include "processors/packet_receiver.hpp"
#include "processors/frame_display.hpp"
#include "processors/h264decoder.hpp"


int main(int argc, const char* argv[]) {
  auto       logger = shar::Logger("client.log");
  const auto opts   = shar::Options::from_args(argc, argv);

  logger.info("Starting with Host: {}, Screen {}x{}",
              opts.ip.to_string(), opts.width, opts.height);

  const shar::Size frame_size {opts.height, opts.width};
  shar::Window     window {frame_size, logger};;

  shar::PacketsQueue received_packets;
  shar::FramesQueue  decoded_frames;

  using Sink = shar::NullQueue<shar::Image>;
  Sink                     sink;
  shar::PacketReceiver     receiver {opts.ip, logger, received_packets};
  shar::H264Decoder        decoder {logger, received_packets, decoded_frames};
  shar::FrameDisplay<Sink> display {window, logger, decoded_frames, sink};

  // start processors
  std::thread receiver_thread {[&] {
    receiver.run();
  }};

  std::thread decoder_thread {[&] {
    decoder.run();
  }};

  // run gui thread
  display.run();

  // stop all processors in reverse order
  display.stop();
  decoder.stop();
  receiver.stop();

  decoder_thread.join();
  receiver_thread.join();

  return EXIT_SUCCESS;
}