#pragma once

#include "channel.hpp"
#include "encoder/ffmpeg/unit.hpp"


namespace shar {

class INetworkModule {
public:
  virtual ~INetworkModule() {}

  virtual void run(Receiver<encoder::ffmpeg::Unit> units) = 0;
  virtual void shutdown() = 0;
};

}