#pragma once

#include "channel.hpp"
#include "packet.hpp"


namespace shar {

class INetworkModule {
public:
  virtual void run(Receiver<Packet> packets) = 0;
  virtual void shutdown() = 0;
};

}