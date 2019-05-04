#include "sender_factory.hpp"
#include "tcp/sender.hpp"
#include "rtp/sender.hpp"


namespace shar {

std::unique_ptr<IPacketSender> create_sender(Context context, Url url) {
  switch (url.protocol()) {
    case Protocol::TCP:
      return std::make_unique<tcp::PacketSender>(std::move(context), url.host(), url.port());
    case Protocol::RTP:
      return std::make_unique<rtp::PacketSender>(std::move(context), url.host(), url.port());
    default:
      assert(false);
    case Protocol::RTSP:
      throw std::runtime_error("Unsupported protocol");
  }
}

}