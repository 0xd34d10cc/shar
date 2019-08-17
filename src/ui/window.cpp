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
    // NOTE: this hacky flag should help with "borderless flag makes window
    //       fuillscreen if size of window is equal to size of screen"
    //       problem, but it doesn't
    SDL_SetHint("SDL_BORDERLESS_WINDOWED_STYLE", "1");
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
    SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN | SDL_WINDOW_ALLOW_HIGHDPI
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

static SDL_HitTestResult hittest_callback(SDL_Window* /*win*/, const SDL_Point* area, void* data) {
  auto* header = reinterpret_cast<Rect*>(data);

  if (header->contains(Point{ area->x, area->y })) {
    return SDL_HITTEST_DRAGGABLE;
  }

  return SDL_HITTEST_NORMAL;
}

Window::Window(const char* name, Size size)
  : m_window(create_window(name, size))
  , m_context(init_gl(m_window.get()))
  , m_header(std::make_unique<Rect>(Rect::empty()))
{
  SDL_SetWindowHitTest(m_window.get(), hittest_callback, m_header.get());
}

Window::~Window() {
  if (m_window) {
    // not sure if it is required
    SDL_SetWindowHitTest(m_window.get(), nullptr, nullptr);
  }
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

void Window::show() {
  SDL_ShowWindow(m_window.get());
}

void Window::minimize() {
  SDL_MinimizeWindow(m_window.get());
}

void Window::set_border(bool active) {
  SDL_SetWindowBordered(m_window.get(), active ? SDL_TRUE : SDL_FALSE);
}

void Window::set_header_area(Rect area) {
  *m_header = area;
}

void Window::swap() {
  SDL_GL_SwapWindow(m_window.get());
}

}