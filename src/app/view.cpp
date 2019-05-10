#include "view.hpp"

#include <cstdlib>

#include "network/receiver_factory.hpp"


namespace shar {

static Context make_context(Options options) {
  auto config = std::make_shared<Options>(std::move(options));
  auto shar_loglvl = config->loglvl;
  if (shar_loglvl > config->encoder_loglvl) {
    throw std::runtime_error("Encoder loglvl mustn't be less than general loglvl");
  }
  auto logger = Logger(config->log_file, shar_loglvl);
  auto registry = std::make_shared<metrics::Registry>();

  return Context{
    std::move(config),
    std::move(logger),
    std::move(registry)
  };
}

static ReceiverPtr make_receiver(Context context) {
  const auto url_str = context.m_config->url;
  const auto url = Url::from_string(url_str);
  return create_receiver(std::move(context), std::move(url));
}

static codec::Decoder make_decoder(Context context) {
  const auto fps = context.m_config->fps;

  return codec::Decoder{
    std::move(context),
    Size{1080, 1920},
    fps
  };
}

static ui::Display make_display(Context context) {
  return ui::Display{
    std::move(context),
    Size{1080, 1920}
  };
}

View::View(Options options)
  : m_context(make_context(std::move(options)))
  , m_receiver(make_receiver(m_context))
  , m_decoder(make_decoder(m_context))
  , m_display(make_display(m_context))
  {}

int View::run() {
  auto [frames_tx, frames_rx] = channel<codec::ffmpeg::Frame>(30);
  auto [packets_tx, packets_rx] = channel<codec::ffmpeg::Unit>(30);

  std::thread receiver_thread{[this,
                               tx{std::move(packets_tx)}] () mutable {
    m_receiver->run(std::move(tx));
  }};

  std::thread decoder_thread{[this,
                              rx{std::move(packets_rx)},
                              tx{std::move(frames_tx)}] () mutable {
    m_decoder.run(std::move(rx), std::move(tx));
  }};

  m_display.run(std::move(frames_rx));

  m_receiver->shutdown();
  m_decoder.shutdown();

  receiver_thread.join();
  decoder_thread.join();

  return EXIT_SUCCESS;
}

}