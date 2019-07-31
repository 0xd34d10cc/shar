#pragma once

#include "context.hpp"
#include "channel.hpp"
#include "cancellation.hpp"
#include "metrics/gauge.hpp"
#include "codec/ffmpeg/frame.hpp"


namespace shar::ui {

class Display: protected Context {
public:
  Display(Context context, Size size);

  void run(Receiver<codec::ffmpeg::Frame> frames);
  void shutdown();

private:
  bool m_gui_enabled{ true };

  Cancellation m_running;
  metrics::Gauge m_fps;
  Size m_size;
};

}