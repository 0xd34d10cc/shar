#include <thread>
#include <stdexcept>
#include <iostream> // replace with logger

#include <boost/program_options.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>

#include "window.hpp"
#include "queues/null_queue.hpp"
#include "queues/frames_queue.hpp"
#include "queues/packets_queue.hpp"
#include "processors/packet_receiver.hpp"
#include "processors/frame_display.hpp"
#include "processors/h264decoder.hpp"

namespace {
  using IpAddress = std::array<std::uint8_t, 4>;
  namespace qi = boost::spirit::qi;
  namespace phx = boost::phoenix;
  qi::uint_parser<std::uint8_t, 10, 1, 3> octet;

  IpAddress make_addr(std::uint8_t o1, std::uint8_t o2, std::uint8_t o3, std::uint8_t o4) {
    return IpAddress{o1, o2, o3, o4};
  }

  struct Ipv4Grammar : qi::grammar<std::string::iterator, IpAddress()> {
    Ipv4Grammar() : Ipv4Grammar::base_type(start) {
      start = (octet >> qi::lit('.') >> octet >> qi::lit('.') >>
        octet >> qi::lit('.') >> octet
        )[
          qi::_val = phx::bind(make_addr, qi::_1, qi::_2, qi::_3, qi::_4)
        ]
        ;
    }
    qi::rule<std::string::iterator, IpAddress()> start;
  } ipv4_grammar;
}

int main(int argc, const char *argv[]) {

  namespace po = boost::program_options;

  po::options_description desc{ "Options" };
  desc.add_options()
    ("help, h", "Help")
    ("Width"  , po::value<size_t>()->default_value(1920), "Width of the screen") //get default values from opengl
    ("Height" , po::value<size_t>()->default_value(1080), "Height of the screen")
    ("Host"   , po::value<std::string>()->default_value("127.0.0.1"), "IP of the server");

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
      std::string input_host = vm["Host"].as<std::string>();
      const bool is_ip = qi::parse(input_host.begin(), input_host.end(), ipv4_grammar, ip);
      if (!is_ip) {
        std::cout << "Wrong Ip addr" << std::endl;
        return 0;
        //throw std::runtime_error("Wrong ip address!");
      }
    }
  }

  const std::size_t width  = vm["Width"].as<size_t>();
  const std::size_t height = vm["Height"].as<size_t>();

  std::cout << "Starting with Host: " << vm["Host"].as<std::string>() << 
                          " Screen: " << width << "x" << height << std::endl;
  
  const shar::Size           frame_size{ height, width };
  shar::Window               window{ frame_size };;

  shar::PacketsQueue         received_packets;
  shar::FramesQueue          decoded_frames;

  using Sink = shar::NullQueue<shar::Image>;
  Sink                       sink;
  shar::PacketReceiver       receiver{ ip, received_packets };
  shar::H264Decoder          decoder{ received_packets, decoded_frames };
  shar::FrameDisplay<Sink>   display{ window, decoded_frames, sink };

  // start processors
  std::thread receiver_thread{ [&] {
    receiver.run();
  } };

  std::thread decoder_thread{ [&] {
    decoder.run();
  } };

  // run gui thread
  display.run();

  // stop all processors in reverse order
  display.stop();
  decoder.stop();
  receiver.stop();

  // FIXME: replace with join() after we figure out how to
  //        notify processors which are waiting on input in the time of shutdown
  decoder_thread.join();
  receiver_thread.join();
  return 0;
}