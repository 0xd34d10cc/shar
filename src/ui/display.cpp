#include "display.hpp"
#include "texture.hpp"

#include "disable_warnings_push.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "disable_warnings_pop.hpp"

#include "codec/convert.hpp"
#include "nk.hpp"
#include "gl_vtable.hpp"
#include "window.hpp"
#include "state.hpp"
#include "renderer.hpp"


static void GLAPIENTRY opengl_error_callback(GLenum /*source*/,
  GLenum type,
  GLuint /*id */,
  GLenum severity,
  GLsizei /*length */,
  const GLchar* message,
  const void* /*userParam */) {
  std::cerr << "[GL]:" << (type == GL_DEBUG_TYPE_ERROR ? " !ERROR! " : "")
    << "type = " << type
    << ", severity = " << severity
    << ", message = " << message
    << std::endl;
}

namespace shar::ui {

Display::Display(Context context, Size size)
    : Context(std::move(context))
    , m_fps({"FPS", "frames per second", "count"}, m_registry)
    , m_window("shar", size)
    , m_renderer(OpenGLVTable::load().value())
    , m_state()
    , m_texture(m_window.size())
{
  glEnable(GL_TEXTURE_2D); // TODO: remove after migration to CORE OpenGL profile
  m_logger.info("OpenGL {}", glGetString(GL_VERSION));
  nk_style_set_font(m_state.context(), m_renderer.default_font_handle());

  // opengl debug output
  if (void* ptr = SDL_GL_GetProcAddress("glDebugMessageCallback")) {
    glEnable(GL_DEBUG_OUTPUT);
    auto debug_output = (PFNGLDEBUGMESSAGECALLBACKARBPROC)ptr;
    debug_output(opengl_error_callback, nullptr);

    m_logger.info("OpenGL debug output enabled");
  }
}

void Display::shutdown() {
  m_running.cancel();
}

void Display::process_input() {
  // process input
  SDL_Event event;
  nk_input_begin(m_state.context());
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      m_running.cancel();
    }

    if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_g) {
      m_gui_enabled = !m_gui_enabled;
    }

    m_state.handle(&event);
  }
  nk_input_end(m_state.context());
}

void Display::process_gui() {
  struct nk_color bg;
  bg.r = 0;
  bg.g = 0;
  bg.b = 0;
  bg.a = 0;

  if (nk_begin(m_state.context(), "Demo", nk_rect(50, 50, 230, 250),
               NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE |
               NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
  {
    enum { EASY, HARD };
    static int op = EASY;
    static int property = 20;

    nk_layout_row_static(m_state.context(), 30, 80, 1);
    if (nk_button_label(m_state.context(), "button"))
      printf("button pressed!\n");

    nk_layout_row_dynamic(m_state.context(), 30, 2);
    if (nk_option_label(m_state.context(), "easy", op == EASY)) op = EASY;
    if (nk_option_label(m_state.context(), "hard", op == HARD)) op = HARD;

    nk_layout_row_dynamic(m_state.context(), 22, 1);
    nk_property_int(m_state.context(), "Compression:", 0, &property, 100, 10, 1);

    nk_layout_row_dynamic(m_state.context(), 20, 1);
    nk_label(m_state.context(), "background:", NK_TEXT_LEFT);

    nk_layout_row_dynamic(m_state.context(), 25, 1);
    if (nk_combo_begin_color(m_state.context(), bg, nk_vec2(nk_widget_width(m_state.context()), 400))) {
      nk_layout_row_dynamic(m_state.context(), 120, 1);
      bg = nk_color_picker(m_state.context(), bg, NK_RGBA);

      nk_layout_row_dynamic(m_state.context(), 25, 1);

      bg.r = 255.0f * nk_propertyf(m_state.context(), "#R:", 0, (float)bg.r / 255.0f, 1.0f, 0.01f, 0.005f);
      bg.g = 255.0f * nk_propertyf(m_state.context(), "#G:", 0, (float)bg.g / 255.0f, 1.0f, 0.01f, 0.005f);
      bg.b = 255.0f * nk_propertyf(m_state.context(), "#B:", 0, (float)bg.b / 255.0f, 1.0f, 0.01f, 0.005f);
      bg.a = 255.0f * nk_propertyf(m_state.context(), "#A:", 0, (float)bg.a / 255.0f, 1.0f, 0.01f, 0.005f);
      nk_combo_end(m_state.context());
    }
  }
  nk_end(m_state.context());
}

void Display::render() {
  const auto win_size = m_window.display_size();
  int width = static_cast<int>(win_size.width());
  int height = static_cast<int>(win_size.height());

  // prepare state
  glViewport(0, 0, width, height);
  glClear(GL_COLOR_BUFFER_BIT);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  // render current video frame
  m_renderer.render(m_texture);

  // render gui
  if (m_gui_enabled) {
    /* IMPORTANT: `Renderer::render()` modifies some global OpenGL state
     * with blending, scissor, face culling, depth test and viewport and
     * defaults everything back into a default state.
     * Make sure to either a.) save and restore or b.) reset your own state after
     * rendering the UI. */
    m_renderer.render(m_state, m_window);
  }

  // finish
  m_window.swap();
}

void Display::run(Receiver<codec::ffmpeg::Frame> frames) {
  while (!m_running.expired() && frames.connected()) {
    if (auto frame = frames.try_receive()) {
      m_texture.bind();
      auto bgra = frame->to_bgra();
      m_texture.update(Point::origin(),
                       frame->sizes(),
                       bgra.data.get());
      m_texture.unbind();
      m_fps.increment();
    }

    process_input();

    if (m_gui_enabled) {
      process_gui();
    }

    render();
    SDL_Delay(1);
  }
}

}