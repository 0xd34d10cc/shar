#include "display.hpp"
#include "texture.hpp"

#include "disable_warnings_push.hpp"
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include <nuklear.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "disable_warnings_pop.hpp"

#include "codec/convert.hpp"
#include "gl_vtable.hpp"
#include "window.hpp"
#include "state.hpp"
#include "renderer.hpp"


// FIXME
#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024


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
    , m_size(size)
    {}

void Display::shutdown() {
  m_running.cancel();
}

void Display::run(Receiver<codec::ffmpeg::Frame> frames) {
  SDL_SetMainReady();

  Window window{"shar", m_size };

  m_logger.info("OpenGL {}", glGetString(GL_VERSION));
  auto vt = OpenGLVTable::load();
  if (!vt) {
    throw std::runtime_error("Failed to load OpenGL vtable");
  }

  Renderer renderer{ std::make_shared<OpenGLVTable>(*vt) };

  int width = static_cast<int>(m_size.width());
  int height = static_cast<int>(m_size.height());
  glViewport(0, 0, width, height);

  // setup gui state
  State state;
  nk_style_set_font(state.context(),
    reinterpret_cast<const nk_user_font*>(renderer.default_font_handle()));

  struct nk_color bg;
  bg.r = 25,
  bg.g = 25,
  bg.b = 25,
  bg.a = 255;

  glEnable(GL_TEXTURE_2D); // TODO: remove after migration to CORE OpenGL profile
  Texture texture{ m_size };

  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT);
  window.swap();

  // opengl debug output
  if (void* ptr = SDL_GL_GetProcAddress("glDebugMessageCallback")) {
    glEnable(GL_DEBUG_OUTPUT);
    auto debug_output = (PFNGLDEBUGMESSAGECALLBACKARBPROC)ptr;
    debug_output(opengl_error_callback, nullptr);

    m_logger.info("OpenGL debug output enabled");
  }

  codec::Slice current_frame;
  Size size{0, 0};

  while (!m_running.expired() && frames.connected()) {
    // process input
    SDL_Event event;
    nk_input_begin(state.context());
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        m_running.cancel();
      }

      if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_g) {
        m_gui_enabled = !m_gui_enabled;
      }

      state.handle(&event);
    }
    nk_input_end(state.context());

    // gui
    if (m_gui_enabled && nk_begin(state.context(), "Demo", nk_rect(50, 50, 230, 250),
      NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE |
      NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
    {
      enum { EASY, HARD };
      static int op = EASY;
      static int property = 20;

      nk_layout_row_static(state.context(), 30, 80, 1);
      if (nk_button_label(state.context(), "button"))
        printf("button pressed!\n");

      nk_layout_row_dynamic(state.context(), 30, 2);
      if (nk_option_label(state.context(), "easy", op == EASY)) op = EASY;
      if (nk_option_label(state.context(), "hard", op == HARD)) op = HARD;

      nk_layout_row_dynamic(state.context(), 22, 1);
      nk_property_int(state.context(), "Compression:", 0, &property, 100, 10, 1);

      nk_layout_row_dynamic(state.context(), 20, 1);
      nk_label(state.context(), "background:", NK_TEXT_LEFT);

      nk_layout_row_dynamic(state.context(), 25, 1);
      if (nk_combo_begin_color(state.context(), bg, nk_vec2(nk_widget_width(state.context()), 400))) {
        nk_layout_row_dynamic(state.context(), 120, 1);
        bg = nk_color_picker(state.context(), bg, NK_RGBA);

        nk_layout_row_dynamic(state.context(), 25, 1);

        bg.r = 255.0f * nk_propertyf(state.context(), "#R:", 0, (float)bg.r / 255.0f, 1.0f, 0.01f, 0.005f);
        bg.g = 255.0f * nk_propertyf(state.context(), "#G:", 0, (float)bg.g / 255.0f, 1.0f, 0.01f, 0.005f);
        bg.b = 255.0f * nk_propertyf(state.context(), "#B:", 0, (float)bg.b / 255.0f, 1.0f, 0.01f, 0.005f);
        bg.a = 255.0f * nk_propertyf(state.context(), "#A:", 0, (float)bg.a / 255.0f, 1.0f, 0.01f, 0.005f);
        nk_combo_end(state.context());
      }
    }

    if (m_gui_enabled)
      nk_end(state.context());

    // process frame
    if (auto frame = frames.try_receive()) {
      current_frame = frame->to_bgra();
      size = frame->sizes();
      m_fps.increment();
    }

    // draw
    int width = static_cast<int>(window.size().width());
    int height = static_cast<int>(window.size().height());

    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor((float)bg.r / 255.0f, (float)bg.g / 255.0f,
                 (float)bg.b / 255.0f, (float)bg.a / 255.0f);


    if (current_frame.data) {
      texture.bind();
      texture.update(Point::origin(),
                     size,
                     current_frame.data.get());
      texture.unbind();
      renderer.render(texture);
    }

    if (m_gui_enabled) {
      /* IMPORTANT: `Renderer::render()` modifies some global OpenGL state
       * with blending, scissor, face culling, depth test and viewport and
       * defaults everything back into a default state.
       * Make sure to either a.) save and restore or b.) reset your own state after
       * rendering the UI. */
      renderer.render(state, window);
    }

    window.swap();
    SDL_Delay(1); // sleep(1ms)
  }
}

}