#include "display.hpp"
#include "texture.hpp"

#include "disable_warnings_push.hpp"
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "disable_warnings_pop.hpp"

#include "codec/convert.hpp"


namespace
{

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
    m_code = SDL_Init(SDL_INIT_EVERYTHING);
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
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    static_cast<int>(size.width()),
    static_cast<int>(size.height()),
    SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
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


void Display::run(Receiver<codec::ffmpeg::Frame> frames) {
  SDL_SetMainReady();

  SDLHandle sdl;
  if (!sdl) {
    throw std::runtime_error(fmt::format("Failed to initialize SDL: {}", SDL_GetError()));
  }

  SDLWindowPtr window = create_window(m_size);
  GLContextPtr gl = init_gl(window.get());
  glEnable(GL_TEXTURE_2D); // TODO: remove after migration to CORE OpenGL profile

  m_logger.info("OpenGL {}", glGetString(GL_VERSION));
  Texture texture{ m_size };

  glViewport(0, 0, m_size.width(), m_size.height());
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT);
  SDL_GL_SwapWindow(window.get());

  if (void* ptr = SDL_GL_GetProcAddress("glDebugMessageCallback")) {
    glEnable(GL_DEBUG_OUTPUT);
    auto debug_output = (PFNGLDEBUGMESSAGECALLBACKARBPROC)ptr;
    debug_output(opengl_error_callback, nullptr);

    m_logger.info("OpenGL debug output enabled");
  }

  texture.bind();
  while (!m_running.expired() && frames.connected()) {
    auto frame = frames.try_receive();
    if (frame) {
      auto bgra = frame->to_bgra();
      texture.update(Point::origin(),
                     frame->sizes(),
                     bgra.data.get());
      draw_texture();
      SDL_GL_SwapWindow(window.get());

      m_fps.increment();
    }

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        m_running.cancel();
      }
    }

    SDL_Delay(1);
  }

  texture.unbind();
}

}