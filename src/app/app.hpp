#pragma once

#include "broadcast.hpp"
#include "cancellation.hpp"
#include "channel.hpp"
#include "codec/ffmpeg/frame.hpp"
#include "config.hpp"
#include "context.hpp"
#include "time.hpp"
#include "ui/button.hpp"
#include "ui/renderer.hpp"
#include "ui/state.hpp"
#include "ui/text_edit.hpp"
#include "ui/texture.hpp"
#include "ui/window.hpp"
#include "view.hpp"
#include "png_reader.hpp"

#include <optional>
#include <variant>

namespace shar {

enum class StreamState { None, Broadcast, View };

class App {
public:
  App(Config config);
  ~App();

  int run();
  StreamState state() const;

private:
  bool process_input();
  bool update_background();
  void update_gui();
  void update_metrics();
  void update_title_bar();
  // update config window
  std::optional<StreamState> update_config();
  void render_background();
  void render();

  void switch_to(StreamState new_state);
  void start_stream();
  void stop_stream();

  // poll for stream error
  // terminates stream if error occured
  void check_stream_state();

  // poll for metrics change
  // returns true if metrics values were updated
  bool check_metrics();

  void save_config();

  Context m_context;
  Cancellation m_running;

  bool m_gui_enabled{true};
  bool m_render_metrics{false};

  ui::Window m_window;
  ui::Renderer m_renderer;
  ui::State m_ui;
  ui::Texture m_background;

  ui::Button m_stop_button;
  ui::Button m_stream_button;
  ui::Button m_view_button;
  ui::TextEdit m_url;

  std::string m_last_error;

  struct MetricsData {
    std::vector<std::string> text;
    TimePoint last_update;
    Milliseconds period;
  };

  Metric m_ticks;
  Metric m_fps;
  MetricsData m_metrics_data;

  struct Empty {
    explicit Empty(Context context) : m_context(context) {}

    void stop() {}

    std::optional<Receiver<BGRAFrame>> start() {
      auto [display_frames_tx, display_frames_rx] = channel<BGRAFrame>(1);
      PNGReader png_reader(std::filesystem::path("D:\\workspace\\shar\\res\\shar.png"), m_logger);
      BGRAFrame background_frame;
      usize n = static_cast<usize>(png_reader.get_height()) * png_reader.get_width() * png_reader.get_channels();

      background_frame.data = std::make_unique<u8[]>(n);
      memcpy(background_frame.data.get(), png_reader.get_data(), n);
      background_frame.size = Size{ static_cast<shar::usize>(png_reader.get_height()), static_cast<shar::usize>(png_reader.get_width()) };
      
      display_frames_tx.send(std::move(background_frame));
      return std::move(display_frames_rx);
    }

    std::string error() const {
      return "";
    }

    Context m_context;
  };

  using Stream = std::variant<Empty, Broadcast, View>;
  Stream m_stream;

  std::optional<Receiver<BGRAFrame>> m_frames;
};

} // namespace shar
