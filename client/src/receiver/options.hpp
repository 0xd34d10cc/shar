#pragma once

#include "disable_warnings_push.hpp"
#include <boost/asio/ip/address.hpp>
#include "disable_warnings_pop.hpp"


using IpAddress = boost::asio::ip::address;

namespace shar {

struct Options {
  IpAddress   ip;
  std::size_t width;
  std::size_t height;

  static Options from_args(int argc, const char* argv[]);
};

}