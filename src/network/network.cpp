#include "network.hpp"
#include "tcp.hpp"
#include "rtp.hpp"

namespace shar {

std::unique_ptr<INetworkModule> create_module(Context context, Url url) {
  switch (url.protocol()) {
    case Protocol::TCP:
      return std::make_unique<tcp::Network>(std::move(context), url.host(), url.port());
    case Protocol::RTP:
      return std::make_unique<rtp::Network>(std::move(context), url.host(), url.port());
    case Protocol::RTSP:
    default:
      throw std::runtime_error("Unsupported protocol");
  }
}

}