#include "window.hpp"

#include <cstdlib>

#include "disable_warnings_push.hpp"
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <fmt/format.h>
#include "disable_warnings_pop.hpp"


namespace shar::ui {

struct SDLHandle {
  SDLHandle() {
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
    m_code = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS);
    SDL_SetMainReady();
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

static SDLHandle SDL_HANDLE;


static Window::SDLWindowPtr create_window(const char* name, shar::Size size) {
  if (!SDL_HANDLE) {
    auto error = fmt::format("Failed to initialize SDL: {}", SDL_GetError());
    throw std::runtime_error(error);

  }

  auto* window = SDL_CreateWindow(
    name,
    100, 100, // position
    static_cast<int>(size.width()),
    static_cast<int>(size.height()),
    SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI
  );

  if (!window) {
    auto error = fmt::format("Failed to create window: {}", SDL_GetError());
    throw std::runtime_error(std::move(error));
  }

  return Window::SDLWindowPtr(window);
}

static Window::GLContextPtr init_gl(SDL_Window* window) {
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

  return Window::GLContextPtr(context);
}

Window::Window(const char* name, Size size)
  : m_window(create_window(name, size))
  , m_context(init_gl(m_window.get()))
{


}

Size Window::size() const {
  int width;
  int height;
  SDL_GetWindowSize(m_window.get(), &width, &height);
  return Size{static_cast<std::size_t>(height),
              static_cast<std::size_t>(width)};
}

Size Window::display_size() const {
  int width;
  int height;
  SDL_GL_GetDrawableSize(m_window.get(), &width, &height);
  return Size{static_cast<std::size_t>(height),
              static_cast<std::size_t>(width)};
}

void Window::swap() {
  SDL_GL_SwapWindow(m_window.get());
}

}