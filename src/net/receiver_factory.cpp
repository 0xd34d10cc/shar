#include "receiver_factory.hpp"

#include "tcp/receiver.hpp"
#include "rtp/receiver.hpp"


namespace shar::net {

std::unique_ptr<IPacketReceiver> create_receiver(Context context, Url url) {
  switch (url.protocol()) {
    case Protocol::TCP:
      return std::make_unique<tcp::PacketReceiver>(std::move(context), url.host(), url.port());
    case Protocol::RTP:
      return std::make_unique<rtp::Receiver>(std::move(context), url.host(), url.port());
    default:
      assert(false);
    case Protocol::RTSP:
      throw std::runtime_error("Unsupported protocol");
  }
}

}