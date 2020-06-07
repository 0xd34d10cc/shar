#include "window.hpp"

#include <cstdlib>
#include <atomic>

#include "disable_warnings_push.hpp"
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_syswm.h>
#include <fmt/format.h>
#ifdef WIN32
#include <dwmapi.h>
#endif
#include "disable_warnings_pop.hpp"


namespace shar::ui {

struct OnMoveData {
  void* handle{ nullptr };
  int timer_id{ 0 };
  bool active{ false };
  std::function<void()> on_move;
};

struct HitTestData {
  SDL_Window* window{ nullptr };
  usize header_height{ 0 };
};


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
    SDL_WINDOW_OPENGL
    | SDL_WINDOW_HIDDEN
    | SDL_WINDOW_ALLOW_HIGHDPI
    | SDL_WINDOW_RESIZABLE
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
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

  SDL_GL_SetSwapInterval(1);

  auto* context = SDL_GL_CreateContext(window);
  if (!context) {
    auto error = fmt::format("Failed to initialize GL context: {}", SDL_GetError());
    throw std::runtime_error(error);
  }

  return Window::GLContextPtr(context);
}

static SDL_HitTestResult hittest_callback(SDL_Window* /*win*/, const SDL_Point* area, void* user_data) {
  auto* data = reinterpret_cast<HitTestData*>(user_data);
  int width;
  int height;
  SDL_GetWindowSize(data->window, &width, &height);

  const int offset = 10;

  const bool right = area->x >= (width - offset);
  const bool left = area->x < offset;
  const bool bottom = area->y > (height - offset);
  const bool top = area->y < offset;

  if (right) {
    return top    ? SDL_HITTEST_RESIZE_TOPRIGHT :
           bottom ? SDL_HITTEST_RESIZE_BOTTOMRIGHT:
                    SDL_HITTEST_RESIZE_RIGHT;
  }
  else if (left) {
    return top    ? SDL_HITTEST_RESIZE_TOPLEFT :
           bottom ? SDL_HITTEST_RESIZE_BOTTOMLEFT :
                    SDL_HITTEST_RESIZE_LEFT;
  }
  else if (top) {
    return SDL_HITTEST_RESIZE_TOP;
  }
  else if (bottom) {
    return SDL_HITTEST_RESIZE_BOTTOM;
  }

  Rect header{
    Point::origin(),
    // -60 for - X buttons
    // TODO: unhardcode
    Size{data->header_height, static_cast<usize>(width - 60)}
  };

  if (header.contains(Point{ static_cast<usize>(area->x), static_cast<usize>(area->y) })) {
    return SDL_HITTEST_DRAGGABLE;
  }

  return SDL_HITTEST_NORMAL;
}

static int handle_event([[maybe_unused]] void* user_data, SDL_Event* event) {
 if (event->type == SDL_SYSWMEVENT) {

#ifdef WIN32
    auto* data = reinterpret_cast<OnMoveData*>(user_data);
    const auto& winMessage = event->syswm.msg->msg.win;
    if (winMessage.msg == WM_ENTERSIZEMOVE) {
      assert(!data->active);
      // the user started dragging, so create the timer (with the minimum timeout)
      // if you have vsync enabled, then this shouldn't render unnecessarily
      data->active = SetTimer((HWND)data->handle, data->timer_id,
                              16 /* ms, ~60 fps, FIXME: should be controlled by callback */,
                              nullptr);
    }
    else if ((winMessage.msg == WM_EXITSIZEMOVE
           || winMessage.msg == WM_CAPTURECHANGED) && data->active) {
      assert(data->active);
      KillTimer((HWND)data->handle, data->timer_id);
      data->active = false;
    }
    else if (winMessage.msg == WM_TIMER && data->active) {
      if (winMessage.wParam == data->timer_id) {
        if (data->on_move) {
          data->on_move();
        }
      }
    }
#endif
  }

  return 0;
}

Window::Window(const char* name, Size size)
  : m_window(create_window(name, size))
  , m_context(init_gl(m_window.get()))
  , m_hittest_data(std::make_unique<HitTestData>())
  , m_move_data(std::make_unique<OnMoveData>())
{
  m_hittest_data->window = m_window.get();
  SDL_SetWindowHitTest(m_window.get(), hittest_callback, m_hittest_data.get());

#ifdef WIN32
  SDL_SysWMinfo wmInfo;
  SDL_VERSION(&wmInfo.version);
  SDL_GetWindowWMInfo(m_window.get(), &wmInfo);
  HWND hwnd = wmInfo.info.win.window;
  m_move_data->handle = hwnd;

  static const MARGINS shadow_state[2]{ { 0,0,0,0 },{ 1,1,1,1 } };
  ::DwmExtendFrameIntoClientArea(hwnd, &shadow_state[true /* enabled */]);
#endif
  static std::atomic<int> timer_id{ 0 };
  m_move_data->timer_id = ++timer_id;

  // register the event watch function
  SDL_AddEventWatch(handle_event, m_move_data.get());
  // we need the native Windows events, so we can listen to WM_ENTERSIZEMOVE and WM_TIMER
  SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);
}

Window::~Window() {
  if (m_window) {
    SDL_SetWindowHitTest(m_window.get(), nullptr, nullptr);
    SDL_DelEventWatch(handle_event, m_move_data.get());
  }
}

Size Window::size() const {
  int width;
  int height;
  SDL_GetWindowSize(m_window.get(), &width, &height);
  return Size{static_cast<usize>(height),
              static_cast<usize>(width)};
}

Size Window::display_size() const {
  int width;
  int height;
  SDL_GL_GetDrawableSize(m_window.get(), &width, &height);
  return Size{static_cast<usize>(height),
              static_cast<usize>(width)};
}

void Window::on_move(std::function<void()> callback) {
  m_move_data->on_move = std::move(callback);
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

void Window::set_header(usize height) {
  m_hittest_data->header_height = height;
}

void Window::swap() {
  SDL_GL_SwapWindow(m_window.get());
}

}
