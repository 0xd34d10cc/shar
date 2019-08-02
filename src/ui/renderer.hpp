#pragma once

#include <memory>

#include "gl_vtable.hpp"


struct nk_font_atlas;
struct nk_user_font;

namespace shar::ui {

struct SDLDevice;
struct OpenGLVTable;
class State;
class Window;
class Texture;

class Renderer {
public:
  Renderer(OpenGLVTable table);
  ~Renderer();

  // render and clear state
  void render(State& state, const Window& window);
  void render(Texture& texture);

  nk_user_font* default_font_handle() const;

private:
  void init();
  void destroy();

  OpenGLVTable m_gl;
  std::unique_ptr<SDLDevice> m_device;
  std::unique_ptr<nk_font_atlas> m_atlas;
};

}