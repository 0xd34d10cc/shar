#include "startup_screen.hpp"

#include "disable_warnings_push.hpp"
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_timer.h>
#include "disable_warnings_pop.hpp"

#include "nk.hpp"


namespace shar::ui {

StartupScreen::StartupScreen()
  : m_window("shar", Size{ 300, 300 })
  , m_renderer(OpenGLVTable::load().value())
  , m_state()
  , m_text(std::make_unique<nk_text_edit>())
{
  nk_style_set_font(m_state.context(), m_renderer.default_font_handle());
  nk_textedit_init_default(m_text.get());
}

StartupScreen::~StartupScreen() {
  nk_textedit_free(m_text.get());
}

void StartupScreen::process_input() {
  // process input
  SDL_Event event;
  nk_input_begin(m_state.context());
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      m_running.cancel();
    }

    m_state.handle(&event);
  }
  nk_input_end(m_state.context());
}

void StartupScreen::render() {
  const auto display = m_window.display_size();
  int width = static_cast<int>(display.width());
  int height = static_cast<int>(display.height());

  glViewport(0, 0, width, height);
  glClear(GL_COLOR_BUFFER_BIT);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  m_renderer.render(m_state, m_window);
  m_window.swap();
}

StartupScreen::Result StartupScreen::run() {
  while (!m_running.expired()) {
    process_input();

    if (nk_begin(m_state.context(), "Startup", nk_rect(0, 0, 300, 300),
                 NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE |
                 NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
    {
      nk_layout_row_static(m_state.context(), 30, 120, 2);
      if (nk_button_label(m_state.context(), "connect")) {
        const char* begin = (const char*)nk_buffer_memory_const(&m_text->string.buffer);
        const char* end = begin + m_text->string.len;

        std::string url{ begin, end };
        return Connect{ std::move(url) };
      }

      if (nk_button_label(m_state.context(), "stream")) {
        const char* begin = (const char*)nk_buffer_memory_const(&m_text->string.buffer);
        const char* end = begin + m_text->string.len;

        std::string url{ begin, end };
        return Stream{ std::move(url) };
      }

      nk_layout_row_static(m_state.context(), 30, 250, 1);
      nk_edit_buffer(m_state.context(), NK_EDIT_DEFAULT | NK_EDIT_ALWAYS_INSERT_MODE,
                     m_text.get(), nk_filter_default);
    }
    nk_end(m_state.context());

    render();
    SDL_Delay(1);
  }

  return Exit{};
}

}