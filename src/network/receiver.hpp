#pragma once

#include "channel.hpp"
#include "encoder/ffmpeg/unit.hpp"


namespace shar {

class IPacketReceiver {
public:
  virtual ~IPacketReceiver() {}

  virtual void run(Sender<encoder::ffmpeg::Unit> units) = 0;
  virtual void shutdown() = 0;
};

}