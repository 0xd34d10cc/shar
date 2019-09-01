#pragma once

#include <memory>
#include <thread>

#include "context.hpp"
#include "channel.hpp"
#include "atomic_string.hpp"
#include "net/receiver.hpp"
#include "codec/decoder.hpp"
#include "codec/ffmpeg/frame.hpp"


namespace shar {

using ReceiverPtr = std::unique_ptr<net::IPacketReceiver>;

class View {
public:
  explicit View(Context context);

  Receiver<codec::ffmpeg::Frame> start();
  void stop();

  bool failed() const;
  std::string error() const;

private:
  Context m_context;

  ReceiverPtr m_receiver;
  codec::Decoder m_decoder;

  std::thread m_network_thread;
  std::thread m_decoder_thread;

  AtomicString m_error;
};

}