#pragma once

#include <memory>
#include <thread>

#include "context.hpp"
#include "channel.hpp"
#include "network/receiver.hpp"
#include "codec/decoder.hpp"
#include "codec/ffmpeg/frame.hpp"


namespace shar {

using ReceiverPtr = std::unique_ptr<IPacketReceiver>;

class View {
public:
  explicit View(Context context);

  Receiver<codec::ffmpeg::Frame> start();
  void stop();

private:
  Context m_context;

  ReceiverPtr m_receiver;
  codec::Decoder m_decoder;

  std::thread m_network_thread;
  std::thread m_decoder_thread;
};

}