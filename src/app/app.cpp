#include <thread>
#include <filesystem>
#include <fstream>

#include "disable_warnings_push.hpp"
#include <SDL2/SDL_events.h>
#include "disable_warnings_pop.hpp"

#include "app.hpp"
#include "env.hpp"
#include "time.hpp"
#include "size.hpp"
#include "ui/gl_vtable.hpp"
#include "ui/nk.hpp"


namespace shar {

namespace fs = std::filesystem;

static Context make_context(Config c) {
  auto config = std::make_shared<Config>(std::move(c));
  auto shar_loglvl = config->log_level;
  if (shar_loglvl > config->encoder_log_level) {
    throw std::runtime_error("Encoder log level should not be less than general log level");
  }

  if (config->log_level != LogLevel::None) {
    auto status = fs::status(config->logs_location);
    if (!fs::exists(status)) {
      if (!fs::create_directories(config->logs_location)) {
        throw std::runtime_error("Failed to create logs directory");
      }
    }
    else if (!fs::is_directory(status)) {
      throw std::runtime_error("logs_location parameter should point to a directory");
    }
  }

  auto logger = Logger(config->logs_location, shar_loglvl);
  auto registry = std::make_shared<metrics::Registry>();

  return Context{
    std::move(config),
    std::move(logger),
    std::move(registry)
  };
}

App::App(Config config)
  : m_context(make_context(std::move(config)))
  // NOTE: should not be equal to screen size, otherwise some
  //       magical SDL kludge makes window fullscreen
  , m_window("shar", Size{ 1080 + 30, 1920 })
  , m_renderer(ui::OpenGLVTable::load().value())
  , m_ui()
  , m_background(m_window.size())
  , m_stop_button("stop")
  , m_stream_button("stream")
  , m_view_button("view")
  , m_url(false, false)
  , m_stream(Empty{})
{
  nk_style_set_font(m_ui.context(), m_renderer.default_font_handle());

  m_url.set_text(m_context.m_config->url);

  Rect area{
    Point::origin(),
    Size{ 30, m_window.display_size().width() - 60 /* - X buttons */ }
  };

  m_window.set_header_area(area);
  m_window.set_border(false);
  m_window.on_move([this] {
    if (update_background()) {
      update_gui();
      render();
    }
  });
  m_window.show();
}

App::~App() {
  stop_stream();
}

bool App::process_input() {
  bool updated = false;

  SDL_Event event;
  nk_input_begin(m_ui.context());
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      m_running.cancel();
    }

    // change gui visibility on shift+f
    if (event.type == SDL_KEYDOWN &&
        event.key.keysym.sym == SDLK_g) {
      const Uint8* state = SDL_GetKeyboardState(0);

      if (state[SDL_SCANCODE_LSHIFT]) {
        m_gui_enabled = !m_gui_enabled;
        updated = true;
      }
    }

    updated |= m_ui.handle(&event);
  }
  nk_input_end(m_ui.context());
  return updated;
}

void App::update_title_bar() {
  Size size = m_window.display_size();

  // title bar
  bool active = nk_begin(m_ui.context(), "shar",
                         nk_rect(0.0f, 0.0f, (float)size.width(), 30.0f),
                         NK_WINDOW_TITLE | NK_WINDOW_CLOSABLE | NK_WINDOW_MINIMIZABLE);
  nk_end(m_ui.context());

  if (!active) {
    if (nk_window_is_closed(m_ui.context(), "shar")) {
      m_running.cancel();
    }

    if (nk_window_is_collapsed(m_ui.context(), "shar")) {
      nk_window_collapse(m_ui.context(), "shar", NK_MAXIMIZED);
      m_window.minimize();
    }
  }
}

std::optional<StreamState> App::update_config() {
  std::optional<StreamState> new_state;

  bool display_error = !m_last_error.empty();
  if (nk_begin(m_ui.context(), "config",
               nk_rect(0.0f, 30.0f, 300.0f, display_error ? 130.0f : 90.0f),
               NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR)) {

    nk_layout_row_dynamic(m_ui.context(), 30, 3);
    if (m_stop_button.process(m_ui)) {
      new_state = StreamState::None;
    }

    if (m_stream_button.process(m_ui)) {
      new_state = StreamState::Broadcast;
    }

    if (m_view_button.process(m_ui)) {
      new_state = StreamState::View;
    }

    nk_layout_row_dynamic(m_ui.context(), 20, 1);
    m_url.process(m_ui);

    const char* state = [this] {
      switch (this->state()) {
        case StreamState::None:
          return "none";
        case StreamState::Broadcast:
          return "broadcast";
        case StreamState::View:
          return "view";
        default:
          assert(false);
          return "none";
      }
    }();

    nk_layout_row_static(m_ui.context(), 20, 70, 2);
    nk_label(m_ui.context(), "state: ", NK_TEXT_LEFT);
    nk_label(m_ui.context(), state, NK_TEXT_LEFT);

    if (display_error) {
      nk_layout_row_dynamic(m_ui.context(), 40, 1);
      nk_text_wrap(m_ui.context(), m_last_error.c_str(),
                   static_cast<int>(m_last_error.size()));
    }
  }
  nk_end(m_ui.context());

  return new_state;
}

void App::render() {
  const auto win_size = m_window.display_size();
  int width = static_cast<int>(win_size.width());
  int height = static_cast<int>(win_size.height());

  // prepare state
  glViewport(0, 0, width, height);
  glClear(GL_COLOR_BUFFER_BIT);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  // render current background (or current frame)
  m_renderer.render(m_background);

  /* IMPORTANT: `Renderer::render()` modifies some global OpenGL state
   * with blending, scissor, face culling, depth test and viewport and
   * defaults everything back into a default state.
   * Make sure to either a.) save and restore or b.) reset your own state after
   * rendering the UI. */
  m_renderer.render(m_ui, m_window);

  // swap buffers
  m_window.swap();
}

void App::switch_to(StreamState new_state) {
  stop_stream();

  switch (new_state) {
    case StreamState::None:
      m_stream.emplace<Empty>();
      break;

    case StreamState::Broadcast:
      m_context.m_config->connect = false;
      m_context.m_config->p2p = true;
      m_context.m_config->url = m_url.text();
      m_stream.emplace<Broadcast>(m_context);
      break;

    case StreamState::View:
      m_context.m_config->connect = true;
      m_context.m_config->url = m_url.text();
      m_stream.emplace<View>(m_context);
      break;
  }

  start_stream();
}

void App::stop_stream() {
  m_frames.reset();

  if (!m_stream.valueless_by_exception()) {
    std::visit([](auto& stream) { stream.stop(); }, m_stream);
  }
}

void App::start_stream() {
  std::visit([this](auto& stream) { m_frames = stream.start(); }, m_stream);
}

bool App::update_background() {
  if (m_frames) {
    if (auto frame = m_frames->try_receive()) {
      m_background.bind();
      m_background.update(Point::origin(),
        frame->size,
        frame->data.get());
      m_background.unbind();
      return true;
    }
  }

  return false;
}

void App::update_gui() {
  update_title_bar();

  try {
    check_stream_state();

    if (m_gui_enabled) {
      if (auto new_state = update_config()) {
        switch_to(*new_state);
        m_last_error.clear();
      }
    }
  }
  catch (const std::exception & e) {
    m_last_error = e.what();

    if (m_stream.valueless_by_exception()) {
      m_stream.emplace<Empty>();
    }
  }
}

int App::run() {
  while (!m_running.expired()) {
    bool updated = process_input();
    updated |= update_background();

    if (updated) {
      update_gui();
      render();
    }
    std::this_thread::sleep_for(Milliseconds(5));
  }

  save_config();
  return EXIT_SUCCESS;
}

void App::check_stream_state() {
  const bool failed = std::visit([this](auto& stream) { return stream.failed(); }, m_stream);

  if (failed) {
    m_last_error = std::visit([this](auto& stream) { return stream.error(); }, m_stream);
    switch_to(StreamState::None);
  }
}

StreamState App::state() const {
  return std::visit([this](auto& stream) {
    using T = std::decay_t<decltype(stream)>;

    if constexpr (std::is_same_v<T, Empty>) {
      return StreamState::None;
    }
    else if constexpr (std::is_same_v<T, Broadcast>) {
      return StreamState::Broadcast;
    }
    else if constexpr (std::is_same_v<T, View>) {
      return StreamState::View;
    }
    else {
      static_assert(false, "types...");
    }

    }, m_stream);
}

void App::save_config() {
  // write only if config file or .shar dir already exist
  bool save = std::filesystem::exists(env::config_path());
  if (!save) {
    if (const auto dir = env::shar_dir()) {
      save = std::filesystem::exists(*dir) && std::filesystem::is_directory(*dir);
    }
  }

  if (save) {
    const auto config = m_context.m_config->to_string();
    std::ofstream out(env::config_path(), std::ios_base::binary);
    out.write(config.data(), config.size());
  }
}

}