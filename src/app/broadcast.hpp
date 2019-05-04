#pragma once

#include <memory>

// TODO: remove direct Exposer dependency from this file
#include "disable_warnings_push.hpp"
#include <prometheus/exposer.h>
#include "disable_warnings_pop.hpp"

#include "common/context.hpp"
#include "capture/capture.hpp"
#include "encoder/encoder.hpp"
#include "network/sender.hpp"


namespace shar {

using SenderPtr = std::unique_ptr<IPacketSender>;
using ExposerPtr = std::unique_ptr<prometheus::Exposer>;

class Broadcast {
public:
  Broadcast(Options options);
  Broadcast(const Broadcast&) = delete;
  Broadcast(Broadcast&&) = delete;
  Broadcast& operator=(const Broadcast&) = delete;
  Broadcast& operator=(Broadcast&&) = delete;
  ~Broadcast() = default;

  int run();

private:
  Context m_context;
  sc::Monitor m_monitor;
  Capture m_capture;
  encoder::Encoder m_encoder;
  SenderPtr m_network;

  ExposerPtr m_exposer;
};

}