#pragma once

#include "channel.hpp"
#include "encoder/ffmpeg/unit.hpp"


namespace shar {

class IPacketSender {
public:
  virtual ~IPacketSender() {}

  virtual void run(Receiver<encoder::ffmpeg::Unit> units) = 0;
  virtual void shutdown() = 0;
};

}