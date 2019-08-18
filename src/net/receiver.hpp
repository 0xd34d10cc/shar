#pragma once

#include "channel.hpp"
#include "codec/ffmpeg/unit.hpp"


namespace shar::net {

class IPacketReceiver {
public:
  virtual ~IPacketReceiver() {}

  virtual void run(Sender<codec::ffmpeg::Unit> units) = 0;
  virtual void shutdown() = 0;
};

}