#pragma once

#include "capture/capture.hpp" // BGRAFrame
#include "channel.hpp"
#include "codec/decoder.hpp"
#include "codec/ffmpeg/frame.hpp"
#include "context.hpp"
#include "net/receiver.hpp"

#include <memory>
#include <thread>

namespace shar {

using ReceiverPtr = std::unique_ptr<net::IPacketReceiver>;

class View {
public:
  explicit View(Context context);

  Receiver<BGRAFrame> start();
  void stop();

  std::string error();

private:
  struct Converter {
    Cancellation m_running;

    void shutdown() {
      m_running.cancel();
    }
  };

  Context m_context;

  ReceiverPtr m_receiver;
  codec::Decoder m_decoder;
  Converter m_converter;

  std::thread m_network_thread;
  std::thread m_decoder_thread;
  std::thread m_converter_thread;

  std::pair<Sender<std::string>, Receiver<std::string>> m_errors;
};

} // namespace shar
