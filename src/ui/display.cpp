#include "display.hpp"
#include "texture.hpp"

#include "disable_warnings_push.hpp"
#include <nuklear.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "disable_warnings_pop.hpp"

#include "gl_vtable.hpp"
#include "codec/convert.hpp"
#include "sdl_backend.hpp"

// FIXME
#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024


namespace {

void GLAPIENTRY opengl_error_callback(GLenum /*source*/,
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

struct SDLHandle {
  SDLHandle() {
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
    m_code = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS);
  }

  ~SDLHandle() {
    if (m_code == 0) {
      SDL_Quit();
    }
  }

  explicit operator bool() {
    return m_code == 0;
  }

  int m_code{ -1 };
};

struct SDLWindowDeleter {
  void operator()(SDL_Window* window) {
    SDL_DestroyWindow(window);
  }
};

using SDLWindowPtr = std::unique_ptr<SDL_Window, SDLWindowDeleter>;

SDLWindowPtr create_window(shar::Size size) {
  auto* window = SDL_CreateWindow("shar",
    100,
    100,
    static_cast<int>(size.width()),
    static_cast<int>(size.height()),
    SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI
  );

  if (!window) {
    auto error = fmt::format("Failed to create window: {}", SDL_GetError());
    throw std::runtime_error(std::move(error));
  }

  return SDLWindowPtr(window);
}

struct GLContextDeleter {
  void operator()(SDL_GLContext context) {
    SDL_GL_DeleteContext(context);;
  }
};

using GLContextPtr = std::unique_ptr<void, GLContextDeleter>;

GLContextPtr init_gl(SDL_Window* window) {
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
  // TODO: use SDL_GL_CONTEXT_PROFILE_CORE
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

  SDL_GL_SetSwapInterval(1);

  auto* context = SDL_GL_CreateContext(window);
  if (!context) {
    auto error = fmt::format("Failed to initialize GL context: {}", SDL_GetError());
    throw std::runtime_error(error);
  }

  return GLContextPtr(context);
}

void draw_texture() {
  // TODO: use shaders
  glBegin(GL_QUADS);

  glTexCoord2f(0, 0);
  glVertex3f(-1, 1, 0);

  glTexCoord2f(0, 1);
  glVertex3f(-1, -1, 0);

  glTexCoord2f(1, 1);
  glVertex3f(1, -1, 0);

  glTexCoord2f(1, 0);
  glVertex3f(1, 1, 0);

  glEnd();
}

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

  SDLHandle sdl;
  if (!sdl) {
    throw std::runtime_error(fmt::format("Failed to initialize SDL: {}", SDL_GetError()));
  }

  SDLWindowPtr window = create_window(m_size);
  GLContextPtr gl = init_gl(window.get());

  m_logger.info("OpenGL {}", glGetString(GL_VERSION));
  auto gl_vtable = OpenGLVTable::load();
  if (!gl_vtable) {
    throw std::runtime_error("Failed to load OpenGL vtable");
  }

  int width = static_cast<int>(m_size.width());
  int height = static_cast<int>(m_size.height());
  glViewport(0, 0, width, height);

  // setup nuklear backend
  SDLBackend* backend = nk_sdl_init(window.get(), &*gl_vtable);
  assert(backend);
  nk_context* ctx = nk_sdl_context(backend);

  struct nk_color bg;
  bg.r = 25,
  bg.g = 25,
  bg.b = 25,
  bg.a = 255;

  // load fonts
  struct nk_font_atlas* atlas;
  nk_sdl_font_stash_begin(backend, &atlas);
  /*struct nk_font *droid = nk_font_atlas_add_from_file(atlas, "../../../extra_font/DroidSans.ttf", 14, 0);*/
  /*struct nk_font *roboto = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Roboto-Regular.ttf", 16, 0);*/
  /*struct nk_font *future = nk_font_atlas_add_from_file(atlas, "../../../extra_font/kenvector_future_thin.ttf", 13, 0);*/
  /*struct nk_font *clean = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyClean.ttf", 12, 0);*/
  /*struct nk_font *tiny = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyTiny.ttf", 10, 0);*/
  /*struct nk_font *cousine = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Cousine-Regular.ttf", 13, 0);*/
  nk_sdl_font_stash_end(backend);

  glEnable(GL_TEXTURE_2D); // TODO: remove after migration to CORE OpenGL profile
  Texture texture{ m_size };

  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT);
  SDL_GL_SwapWindow(window.get());

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
    nk_input_begin(ctx);
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        m_running.cancel();
      }

      if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_g) {
        m_gui_enabled = !m_gui_enabled;
      }

      nk_sdl_handle_event(backend, &event);
    }
    nk_input_end(ctx);

    // gui
    if (m_gui_enabled && nk_begin(ctx, "Demo", nk_rect(50, 50, 230, 250),
      NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE |
      NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
    {
      enum { EASY, HARD };
      static int op = EASY;
      static int property = 20;

      nk_layout_row_static(ctx, 30, 80, 1);
      if (nk_button_label(ctx, "button"))
        printf("button pressed!\n");

      nk_layout_row_dynamic(ctx, 30, 2);
      if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
      if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;

      nk_layout_row_dynamic(ctx, 22, 1);
      nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);

      nk_layout_row_dynamic(ctx, 20, 1);
      nk_label(ctx, "background:", NK_TEXT_LEFT);

      nk_layout_row_dynamic(ctx, 25, 1);
      if (nk_combo_begin_color(ctx, bg, nk_vec2(nk_widget_width(ctx), 400))) {
        nk_layout_row_dynamic(ctx, 120, 1);
        bg = nk_color_picker(ctx, bg, NK_RGBA);

        nk_layout_row_dynamic(ctx, 25, 1);

        bg.r = 255.0f * nk_propertyf(ctx, "#R:", 0, (float)bg.r / 255.0f, 1.0f, 0.01f, 0.005f);
        bg.g = 255.0f * nk_propertyf(ctx, "#G:", 0, (float)bg.g / 255.0f, 1.0f, 0.01f, 0.005f);
        bg.b = 255.0f * nk_propertyf(ctx, "#B:", 0, (float)bg.b / 255.0f, 1.0f, 0.01f, 0.005f);
        bg.a = 255.0f * nk_propertyf(ctx, "#A:", 0, (float)bg.a / 255.0f, 1.0f, 0.01f, 0.005f);
        nk_combo_end(ctx);
      }
    }

    if (m_gui_enabled)
      nk_end(ctx);

    // process frame
    if (auto frame = frames.try_receive()) {
      current_frame = frame->to_bgra();
      size = frame->sizes();
      m_fps.increment();
    }

    // draw
    SDL_GetWindowSize(window.get(), &width, &height);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor((float)bg.r / 255.0f, (float)bg.g / 255.0f,
                 (float)bg.b / 255.0f, (float)bg.a / 255.0f);


    if (current_frame.data) {
      texture.bind();
      texture.update(Point::origin(),
                     size,
                     current_frame.data.get());
      draw_texture();
      texture.unbind();
    }

    if (m_gui_enabled) {
     /* IMPORTANT: `nk_sdl_render` modifies some global OpenGL state
      * with blending, scissor, face culling, depth test and viewport and
      * defaults everything back into a default state.
      * Make sure to either a.) save and restore or b.) reset your own state after
      * rendering the UI. */
      nk_sdl_render(backend, &*gl_vtable, NK_ANTI_ALIASING_ON,
                    MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY);
    }

    SDL_GL_SwapWindow(window.get());


    SDL_Delay(1);
  }
}

}