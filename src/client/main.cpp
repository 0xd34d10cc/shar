#include <thread>
#include <iostream> // replace with logger

#include "disable_warnings_push.hpp"
#include <boost/program_options.hpp>
#include <boost/asio/ip/address.hpp>
#include "disable_warnings_pop.hpp"

#include "logger.hpp"
#include "window.hpp"
#include "queues/null_queue.hpp"
#include "queues/frames_queue.hpp"
#include "queues/packets_queue.hpp"
#include "processors/packet_receiver.hpp"
#include "processors/frame_display.hpp"
#include "processors/h264decoder.hpp"

int main(int argc, const char* argv[]) {

  namespace po = boost::program_options;
  using IpAddress = boost::asio::ip::address;

  auto logger = shar::Logger("client.log");
  po::options_description desc {"Options"};
  desc.add_options()
          ("help, h", "Help")
          ("width", po::value<size_t>()->default_value(1920), "Width of the screen") //get default values from opengl
          ("height", po::value<size_t>()->default_value(1080), "Height of the screen")
          ("host", po::value<std::string>()->default_value("127.0.0.1"), "IP of the server");

  po::variables_map vm;
  po::store(parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  IpAddress ip;
  { // Parse input from user
    if (vm.count("help")) {
      std::cout << desc << '\n';
      return 0;
    }
    else {
      const std::string input_host = vm["host"].as<std::string>();
      boost::system::error_code ec;
      ip = boost::asio::ip::address::from_string(input_host, ec);
      if (ec != boost::system::errc::success) {
        std::cerr << "Invalid Ip" << std::endl;
        return 0;
      }
    }
  }

  const std::size_t width  = vm["width"].as<size_t>();
  const std::size_t height = vm["height"].as<size_t>();

  logger.info("Starting with Host: {0}, Screen {1}x{2}", vm["host"].as<std::string>(), width, height);

  const shar::Size frame_size {height, width};
  shar::Window     window {frame_size, logger};;

  shar::PacketsQueue received_packets;
  shar::FramesQueue  decoded_frames;

  using Sink = shar::NullQueue<shar::Image>;
  Sink                     sink;
  shar::PacketReceiver     receiver {ip, logger, received_packets};
  shar::H264Decoder        decoder {received_packets, logger, decoded_frames};
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
  return 0;
}