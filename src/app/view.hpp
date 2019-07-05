#pragma once

#include <memory>

#include "disable_warnings_push.hpp"
#include <prometheus/exposer.h>
#include "disable_warnings_pop.hpp"

#include "options.hpp"
#include "network/receiver.hpp"
#include "codec/decoder.hpp"
#include "ui/display.hpp"


namespace shar {

using ReceiverPtr = std::unique_ptr<IPacketReceiver>;

class View {
public:
  explicit View(Options options);

  int run();

private:
  Context m_context;

  ReceiverPtr m_receiver;
  codec::Decoder m_decoder;
  ui::Display m_display;
};

}