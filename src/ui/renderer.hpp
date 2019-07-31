#pragma once

#include <memory>


struct nk_font_atlas;

namespace shar::ui {

struct SDLDevice;
struct OpenGLVTable;
class State;
class Window;
class Texture;

class Renderer {
public:
  Renderer(std::shared_ptr<OpenGLVTable> table);
  ~Renderer();

  // render and clear state
  void render(State& state, const Window& window);
  void render(Texture& texture);

  void* default_font_handle() const;

private:
  void init();
  void destroy();

  std::shared_ptr<OpenGLVTable> m_gl;
  std::unique_ptr<SDLDevice> m_device;
  std::unique_ptr<nk_font_atlas> m_atlas;
};

}