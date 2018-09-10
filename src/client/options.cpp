#include <iostream>

#include "disable_warnings_push.hpp"
#include <boost/program_options.hpp>
#include "disable_warnings_pop.hpp"

#include "options.hpp"


namespace po = boost::program_options;

namespace {
static shar::Options parse_options(const po::variables_map& vm) {
  const std::string         input_host = vm["host"].as<std::string>();
  boost::system::error_code ec;

  const auto ip = IpAddress::from_string(input_host);

  const std::size_t width  = vm["width"].as<size_t>();
  const std::size_t height = vm["height"].as<size_t>();

  return shar::Options {ip, width, height};
}

static shar::Options read_options(int argc, const char* argv[]) {
  po::options_description desc {"Options"};
  desc.add_options()
          ("help,h", "Help")
          // TODO: use screen size for default
          ("width", po::value<size_t>()->default_value(1920), "Width of the screen")
          ("height", po::value<size_t>()->default_value(1080), "Height of the screen")
          ("host", po::value<std::string>()->default_value("127.0.0.1"), "IP of the server");

  po::variables_map vm;
  po::store(parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    std::exit(EXIT_SUCCESS);
  }

  try {
    return parse_options(vm);
  } catch (const std::exception& e) {
    std::cerr << "Failed to parse command line options: " << e.what() << std::endl;
    std::exit(EXIT_FAILURE);
  }
}
}

namespace shar {

Options Options::from_args(int argc, const char* argv[]) {
  return read_options(argc, argv);
}

}