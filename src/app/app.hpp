#pragma once

#include <memory>

// TODO: remove direct Exposer dependency from this file
#include "disable_warnings_push.hpp"
#include <prometheus/exposer.h>
#include "disable_warnings_pop.hpp"

#include "common/context.hpp"
#include "capture/capture.hpp"
#include "encoder/encoder.hpp"
#include "network/module.hpp"


namespace shar {

using NetworkPtr = std::unique_ptr<INetworkModule>;
using ExposerPtr = std::unique_ptr<prometheus::Exposer>;

class App {
public:
  // TODO: replace |argc| and |argv| with Options object
  App(int argc, const char* argv[]);
  App(const App&) = delete;
  App(App&&) = delete;
  App& operator=(const App&) = delete;
  App& operator=(App&&) = delete;
  ~App() = default;

  int run();

private:
  Context m_context;
  sc::Monitor m_monitor;
  Capture m_capture;
  encoder::Encoder m_encoder;
  NetworkPtr m_network;

  ExposerPtr m_exposer;
};

}