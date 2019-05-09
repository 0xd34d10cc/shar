#pragma once

#include "channel.hpp"
#include "codec/ffmpeg/unit.hpp"


namespace shar {

class IPacketSender {
public:
  virtual ~IPacketSender() {}

  virtual void run(Receiver<codec::ffmpeg::Unit> units) = 0;
  virtual void shutdown() = 0;
};

}