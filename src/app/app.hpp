#pragma once

#include <variant>
#include <optional>

#include "channel.hpp"
#include "config.hpp"
#include "context.hpp"
#include "cancellation.hpp"

#include "ui/window.hpp"
#include "ui/renderer.hpp"
#include "ui/state.hpp"
#include "ui/texture.hpp"
#include "ui/text_edit.hpp"
#include "ui/button.hpp"

#include "codec/ffmpeg/frame.hpp"

#include "broadcast.hpp"
#include "view.hpp"


namespace shar {

enum class StreamState {
  None,
  Broadcast,
  View
};

class App {
public:
  App(Config config);
  ~App();

  int run();
  StreamState state() const;

private:
  void tick();
  void process_input();
  void process_title_bar();
  std::optional<StreamState> process_gui();
  void render();

  void switch_to(StreamState new_state);
  void stop_stream();
  void start_stream();
  void check_stream_state();

  void save_config();

  Context m_context;
  Cancellation m_running;

  bool m_gui_enabled{ true };

  ui::Window m_window;
  ui::Renderer m_renderer;
  ui::State m_ui;
  ui::Texture m_background;

  ui::Button m_stop_button;
  ui::Button m_stream_button;
  ui::Button m_view_button;
  ui::TextEdit m_url;
  std::string m_last_error;

  struct Empty {
    void stop() {}

    std::optional<Receiver<codec::ffmpeg::Frame>> start() {
      // TODO: replace by once<Frame>(Frame::black())
      return std::nullopt;
    }

    bool failed() const {
      return false;
    }

    std::string error() const {
      return "";
    }
  };

  using Stream = std::variant<Empty, Broadcast, View>;
  Stream m_stream;

  std::optional<Receiver<codec::ffmpeg::Frame>> m_frames;
};

}