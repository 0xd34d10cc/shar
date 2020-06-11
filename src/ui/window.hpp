#pragma once

#include <string>
#include <memory>
#include <functional>

#include "disable_warnings_push.hpp"
#include <SDL2/SDL_video.h>
#include "disable_warnings_pop.hpp"

#include "size.hpp"
#include "rect.hpp"


namespace shar::ui {

struct OnMoveData;
struct HitTestData;

// Handle to SDL window and OpenGL context
class Window {
public:
  Window(const char* name, Size size);
  Window(Window&&) = default;
  Window& operator=(Window&&) = default;
  ~Window();

  Size size() const;
  Size display_size() const;

  void set_border(bool active);
  void set_header(usize height);

  void show();
  void minimize();

  bool is_fullscreen() const noexcept;
  void set_fullscreen(bool fullscreen);
  void on_move(std::function<void()> callback);

  void swap();

  struct SDLWindowDeleter {
    void operator()(SDL_Window* window) {
      SDL_DestroyWindow(window);
    }
  };

  using SDLWindowPtr = std::unique_ptr<SDL_Window, SDLWindowDeleter>;

  struct GLContextDeleter {
    void operator()(SDL_GLContext context) {
      SDL_GL_DeleteContext(context);
    }
  };

  using GLContextPtr = std::unique_ptr<void, GLContextDeleter>;

private:

  bool m_fullscreen{ false };
  SDLWindowPtr m_window;
  GLContextPtr m_context;
  std::unique_ptr<HitTestData> m_hittest_data;
  std::unique_ptr<OnMoveData> m_move_data;
};

}