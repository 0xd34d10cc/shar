#pragma once

#include "disable_warnings_push.hpp"
#include <boost/asio/ip/address.hpp>
#include "disable_warnings_pop.hpp"
#include "network/consts.hpp"

using IpAddress = boost::asio::ip::address;
using Port = const std::uint16_t;
namespace shar {

struct Options {
  IpAddress   ip;
  Port port;
  std::size_t width;
  std::size_t height;

  static Options from_args(int argc, const char* argv[]);
};

}