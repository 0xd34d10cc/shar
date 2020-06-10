#pragma once

#include <memory>

#include "size.hpp"
#include "point.hpp"
#include "gl_vtable.hpp"


struct nk_font_atlas;
struct nk_user_font;

namespace shar {
class Logger;
}

namespace shar::ui {

struct SDLDevice;
struct OpenGLVTable;
class State;
class Window;
class Texture;

using AttributeID = u32;

struct ShaderAttributes {
  AttributeID position{ 0 };
  AttributeID texture_coordinates{ 0 };
  AttributeID color{ 0 };

  AttributeID projection{ 0 };
  AttributeID texture{ 0 };
};

using ProgramID = u32;

struct Shader {
  ProgramID vertex{ 0 };
  ProgramID fragment{ 0 };
  ProgramID program{ 0 };

  ShaderAttributes attributes;
};


class Renderer {
public:
  static void init_log(const Logger& logger);

  Renderer(OpenGLVTable table);
  ~Renderer();

  // render and clear state
  // TODO: decouple command list from State
  void render(State& state, const Window& window);
  void render(Texture& texture, const Window& window,
              Point at, usize x_offset, usize y_offset);

  nk_user_font* default_font_handle() const;

  // compile shader from sources
  Shader compile(const char* vertex_shader, const char* fragment_shader);

  // free shader and associated resources
  void free(Shader& shader);

private:
  void init();
  void destroy();

  OpenGLVTable m_gl;
  Shader m_shader;

  std::unique_ptr<SDLDevice> m_device;
  std::unique_ptr<nk_font_atlas> m_atlas;
};

}