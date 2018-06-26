#include <thread>
#include <iostream>
#include <chrono>

#include "window.hpp"
#include "queues/null_queue.hpp"
#include "queues/frames_queue.hpp"
#include "queues/packets_queue.hpp"
#include "processors/packet_sender.hpp"
#include "processors/packet_receiver.hpp"
#include "processors/capture_frame_provider.hpp"
#include "processors/frame_display.hpp"
#include "processors/h264encoder.hpp"
#include "processors/h264decoder.hpp"


namespace sc = SL::Screen_Capture;

// "client":
//    PacketReceiver[Server => Packets]
//      -> H264Decoder[Packets => Frames]
//        -> FrameDisplay[Frames => NULL]

// "server":
//    CaptureFrameProvider[WindowManager => Frames]
//      -> FrameDisplay[Frames => Frames]
//        -> H264Encoder[Frames => Packets]
//           -> PacketsSender

int main() {
  // TODO: make it configurable
  auto        monitor = sc::GetMonitors().front();
  std::size_t width   = static_cast<std::size_t>(monitor.Width);
  std::size_t height  = static_cast<std::size_t>(monitor.Height);
  std::cout << "Capturing " << monitor.Name << " " << width << 'x' << height << std::endl;

  shar::Size   frame_size {height, width};
  shar::Window window {frame_size};;

  using namespace std::chrono_literals;
  const int  fps      = 30;
  const auto interval = 1000ms / fps;

  shar::FramesQueue  captured_frames;
  shar::PacketsQueue packets_to_send;
  shar::PacketsQueue received_packets;
  shar::FramesQueue  decoded_frames;

  // setup processors pipeline
  shar::CaptureFrameProvider capture {interval, monitor, captured_frames};
  shar::H264Encoder          encoder {frame_size, 11000 * 1024 /* bitrate */,
                                      captured_frames, packets_to_send};
  shar::PacketSender         sender {packets_to_send};
  shar::PacketReceiver       receiver {{127, 0, 0, 1}, received_packets};
  shar::H264Decoder          decoder {received_packets, decoded_frames};

  using Sink = shar::NullQueue<shar::Image>;
  using Display = shar::FrameDisplay<Sink>;
  Sink    sink;
  Display display {window, decoded_frames, sink};

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

  // FIXME: for some reason this sleep breaks pipeline
  // NOTE: most likely some queue gets overflown
  // std::this_thread::sleep_for(500ms);

  std::thread receiver_thread {[&] {
    receiver.run();
  }};

  std::thread decoder_thread {[&] {
    decoder.run();
  }};

  // run gui thread
  display.run();

  // stop all processors
  capture.stop();
  encoder.stop();
  sender.stop();
  receiver.stop();
  decoder.stop();
  display.stop();

  capture_thread.join();
  encoder_thread.join();
  sender_thread.join();
  receiver_thread.join();
  decoder_thread.join();

  return 0;
}