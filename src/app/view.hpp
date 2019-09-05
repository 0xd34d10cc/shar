#pragma once

#include <memory>
#include <thread>

#include "context.hpp"
#include "channel.hpp"
#include "atomic_string.hpp"
#include "net/receiver.hpp"
#include "codec/decoder.hpp"
#include "codec/ffmpeg/frame.hpp"
#include "capture/capture.hpp" // BGRAFrame


namespace shar {

using ReceiverPtr = std::unique_ptr<net::IPacketReceiver>;

class View {
public:
  explicit View(Context context);

  Receiver<BGRAFrame> start();
  void stop();

  bool failed() const;
  std::string error() const;

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

  AtomicString m_error;
};

}