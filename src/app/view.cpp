#include "view.hpp"

#include <cstdlib>

#include "net/receiver_factory.hpp"


namespace shar {

static ReceiverPtr make_receiver(Context context) {
  auto url_str = context.m_config->url;
  auto url = net::Url::from_string(url_str);
  return create_receiver(std::move(context), std::move(url));
}

static codec::Decoder make_decoder(Context context) {
  const auto fps = context.m_config->fps;

  return codec::Decoder{
    std::move(context),
    // FIXME: unhardcode
    Size{1080, 1920},
    fps
  };
}

View::View(Context context)
  : m_context(context)
  , m_receiver(make_receiver(m_context))
  , m_decoder(make_decoder(m_context))
  {}

Receiver<codec::ffmpeg::Frame> View::start() {
  auto [frames_tx, frames_rx] = channel<codec::ffmpeg::Frame>(30);
  auto [packets_tx, packets_rx] = channel<codec::ffmpeg::Unit>(30);

  m_network_thread = std::thread{[this,
                                  tx{std::move(packets_tx)}] () mutable {
    try {
      m_receiver->run(std::move(tx));
    }
    catch (const std::exception& e) {
      m_error.set(e.what());
    }
  }};

  m_decoder_thread = std::thread{[this,
                                  rx{std::move(packets_rx)},
                                  tx{std::move(frames_tx)}] () mutable {
    try {
      m_decoder.run(std::move(rx), std::move(tx));
    }
    catch (const std::exception& e) {
      m_error.set(e.what());
    }
  }};

  return std::move(frames_rx);
}

void View::stop() {
  m_receiver->shutdown();
  m_decoder.shutdown();

  m_network_thread.join();
  m_decoder_thread.join();
}

bool View::failed() const {
  return m_error.initialized();
}

std::string View::error() const {
  return m_error.get();
}

} // namespace shar