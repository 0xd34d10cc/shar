#pragma once

#include <memory>
#include <thread>

#include "context.hpp"
#include "channel.hpp"
#include "capture/capture.hpp"
#include "net/sender.hpp"
#include "codec/encoder.hpp"
#include "codec/ffmpeg/frame.hpp"


namespace shar {

using SenderPtr = std::unique_ptr<net::IPacketSender>;

class Broadcast {
public:
  explicit Broadcast(Context context);
  Broadcast(const Broadcast&) = delete;
  Broadcast(Broadcast&&) = delete;
  Broadcast& operator=(const Broadcast&) = delete;
  Broadcast& operator=(Broadcast&&) = delete;
  ~Broadcast() = default;

  Receiver<codec::ffmpeg::Frame> start();
  void stop();

private:
  Context m_context;

  sc::Monitor m_monitor;
  Capture m_capture;
  codec::Encoder m_encoder;
  SenderPtr m_network;

  std::thread m_encoder_thread;
  std::thread m_network_thread;
};

}