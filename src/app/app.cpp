#include "app.hpp"

#include <thread>
#include <chrono>

#include "disable_warnings_push.hpp"
#include <SDL2/SDL_events.h>
#include "disable_warnings_pop.hpp"

#include "size.hpp"
#include "ui/gl_vtable.hpp"
#include "ui/nk.hpp"


namespace shar {

static Context make_context(Options options) {
  auto config = std::make_shared<Options>(std::move(options));
  auto shar_loglvl = config->loglvl;
  if (shar_loglvl > config->encoder_loglvl) {
    throw std::runtime_error("Encoder log level be less than general log level");
  }

  auto logger = Logger(config->log_file, shar_loglvl);
  auto registry = std::make_shared<metrics::Registry>();

  return Context{
    std::move(config),
    std::move(logger),
    std::move(registry)
  };
}

App::App(Options options)
  : m_context(make_context(options))
  , m_window("shar", Size{ 1080, 1920 })
  , m_renderer(ui::OpenGLVTable::load().value())
  , m_ui()
  , m_background(m_window.size())
  , m_state()
{
  nk_style_set_font(m_ui.context(), m_renderer.default_font_handle());
}

App::~App() {
  if (!m_state.valueless_by_exception()) {
    std::visit([](auto& state) { state.stop(); }, m_state);
  }
}

void App::process_input() {
  SDL_Event event;
  nk_input_begin(m_ui.context());
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      m_running.cancel();
    }

    // change gui visibility on shift+f
    if (event.type == SDL_KEYDOWN &&
        event.key.keysym.sym == SDLK_f) {
      const Uint8* state = SDL_GetKeyboardState(0);

      if (state[SDL_SCANCODE_LSHIFT]) {
        m_gui_enabled = !m_gui_enabled;
      }
    }

    m_ui.handle(&event);
  }
  nk_input_end(m_ui.context());
}

void App::process_gui() {
  Size size = m_window.display_size();

  const auto stop = [this] {
    std::visit([](auto& state) { state.stop(); }, m_state);
  };

  const auto start = [this] {
    std::visit([this](auto& state) { m_frames = state.start(); }, m_state);
  };

  if (nk_begin(m_ui.context(), "Config",
               nk_rect(0, 0, 300, size.height()),
               NK_WINDOW_BORDER)) {

    nk_layout_row_static(m_ui.context(), 30, 80, 3);
    if (nk_button_label(m_ui.context(), "stream")) {
      stop();

      auto context = m_context;
      context.m_config->connect = false;
      context.m_config->p2p = true;
      context.m_config->url = m_url.text();
      m_state.emplace<Broadcast>(std::move(context));

      start();
    }

    if (nk_button_label(m_ui.context(), "view")) {
      stop();

      auto context = m_context;
      context.m_config->connect = true;
      context.m_config->url = m_url.text();
      m_state.emplace<View>(std::move(context));

      start();
    }

    if (nk_button_label(m_ui.context(), "stop")) {
      stop();
      m_state.emplace<Empty>();
      start();
    }

    nk_layout_row_static(m_ui.context(), 20, 250, 1);
    m_url.process(m_ui);

    nk_layout_row_static(m_ui.context(), 20, 250, 1);
    const char* state = std::visit([this](auto& state) {
      using T = std::decay_t<decltype(state)>;

      if constexpr (std::is_same_v<T, Empty>) {
        return "none";
      }
      else if constexpr (std::is_same_v<T, Broadcast>) {
        return "stream";
      }
      else if constexpr (std::is_same_v<T, View>) {
        return "view";
      }
      else {
        static_assert(false, "types...");
      }

    }, m_state);

    nk_label(m_ui.context(), state, NK_TEXT_LEFT);
  }
  nk_end(m_ui.context());
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

  // render gui
  if (m_gui_enabled) {
    /* IMPORTANT: `Renderer::render()` modifies some global OpenGL state
     * with blending, scissor, face culling, depth test and viewport and
     * defaults everything back into a default state.
     * Make sure to either a.) save and restore or b.) reset your own state after
     * rendering the UI. */
    m_renderer.render(m_ui, m_window);
  }

  // finish
  m_window.swap();
}

int App::run() {
  while (!m_running.expired()) {

    if (m_frames) {
      if (auto frame = m_frames->try_receive()) {
        m_background.bind();
        auto bgra = frame->to_bgra();
        m_background.update(Point::origin(),
          frame->sizes(),
          bgra.data.get());
        m_background.unbind();
      }
    }

    process_input();

    if (m_gui_enabled) {
      process_gui();
    }

    render();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }

  return EXIT_SUCCESS;
}

}