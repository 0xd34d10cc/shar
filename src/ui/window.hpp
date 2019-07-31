#pragma once

#include <string>
#include <memory>

#include "disable_warnings_push.hpp"
#include <SDL2/SDL_video.h>
#include "disable_warnings_pop.hpp"

#include "size.hpp"


namespace shar::ui {

// Handle to SDL window and OpenGL context
class Window {
public:
  Window(const char* name, Size size);
  Window(Window&&) = default;
  Window& operator=(Window&&) = default;
  ~Window() = default;

  Size size() const;
  Size display_size() const;

  void swap();


  struct SDLWindowDeleter {
    void operator()(SDL_Window* window) {
      SDL_DestroyWindow(window);
    }
  };

  using SDLWindowPtr = std::unique_ptr<SDL_Window, SDLWindowDeleter>;

  struct GLContextDeleter {
    void operator()(SDL_GLContext context) {
      SDL_GL_DeleteContext(context);;
    }
  };

  using GLContextPtr = std::unique_ptr<void, GLContextDeleter>;

private:
  SDLWindowPtr m_window;
  GLContextPtr m_context;
};

}