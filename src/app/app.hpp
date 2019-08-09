#pragma once

#include <variant>
#include <optional>

#include "channel.hpp"
#include "options.hpp"
#include "context.hpp"
#include "cancellation.hpp"

#include "ui/window.hpp"
#include "ui/renderer.hpp"
#include "ui/state.hpp"
#include "ui/texture.hpp"
#include "ui/text_edit.hpp"

#include "codec/ffmpeg/frame.hpp"

#include "broadcast.hpp"
#include "view.hpp"


namespace shar {

class App {
public:
  App(Options options);
  ~App();

  int run();

private:
  void process_input();
  void process_gui();
  void render();

  Context m_context;
  Cancellation m_running;

  bool m_gui_enabled{ true };

  ui::Window m_window;
  ui::Renderer m_renderer;
  ui::State m_ui;
  ui::Texture m_background;
  ui::TextEdit m_url;

  struct Empty {
    void stop() {}
    std::optional<Receiver<codec::ffmpeg::Frame>> start() {
      // TODO: replace by once<Frame>(Frame::black())
      return std::nullopt;
    }
  };

  using State = std::variant<Empty, Broadcast, View>;
  State m_state;

  std::optional<Receiver<codec::ffmpeg::Frame>> m_frames;
};

}