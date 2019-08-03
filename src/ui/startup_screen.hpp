#pragma once

#include <variant>
#include <memory>

#include "window.hpp"
#include "renderer.hpp"
#include "state.hpp"
#include "cancellation.hpp"


struct nk_text_edit;

namespace shar::ui {

class StartupScreen {
public:
  StartupScreen();
  ~StartupScreen();

  struct Exit {};
  struct Stream {
    std::string url;
  };
  struct Connect {
    std::string url;
  };

  using Result = std::variant<Exit, Stream, Connect>;

  Result run();

private:
  void process_input();
  void render();

  Window m_window;
  Renderer m_renderer;
  State m_state;

  std::unique_ptr<nk_text_edit> m_text;
  Cancellation m_running;
};

}