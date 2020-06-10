#include "renderer.hpp"

#include "gl_vtable.hpp"
#include "logger.hpp"
#include "nk.hpp"
#include "state.hpp"
#include "texture.hpp"
#include "window.hpp"

#include <cassert>
#include <cstddef>
#include <cstdlib> // memset
#include <optional>

// clang-format off
#include "disable_warnings_push.hpp"
#include <SDL2/SDL_opengl_glext.h>
#include "disable_warnings_pop.hpp"
// clang-format on


static const nk_size MAX_VERTEX_MEMORY = 512 * 1024;
static const nk_size MAX_ELEMENT_MEMORY = 128 * 1024;

#ifdef __APPLE__
#define SHAR_SHADER_VERSION "#version 150\n"
#else
#define SHAR_SHADER_VERSION "#version 300 es\n"
#endif

#ifdef SHAR_DEBUG_BUILD
static std::optional<shar::Logger> gl_logger;

static const char *type_to_str(GLenum type) {
  switch (type) {
    case GL_DEBUG_TYPE_ERROR:
      return "error";
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
      return "deprecated";
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
      return "UB";
    case GL_DEBUG_TYPE_PORTABILITY:
      return "unportable";
    case GL_DEBUG_TYPE_PERFORMANCE:
      return "perf";
    case GL_DEBUG_TYPE_MARKER:
      return "cmdstream";
    case GL_DEBUG_TYPE_PUSH_GROUP:
      return "begin";
    case GL_DEBUG_TYPE_POP_GROUP:
      return "end";
    case GL_DEBUG_TYPE_OTHER:
      return "other";
    default:
      return "unknown";
  }
}

static void GLAPIENTRY opengl_error_callback(GLenum /*source*/,
                                             GLenum type,
                                             GLuint /*id */,
                                             GLenum severity,
                                             GLsizei /*length */,
                                             const GLchar *message,
                                             const void * /*userParam */) {
  if (gl_logger) {
    auto *t = type_to_str(type);
    switch (severity) {
      case GL_DEBUG_SEVERITY_LOW:
      case GL_DEBUG_SEVERITY_MEDIUM:
      case GL_DEBUG_SEVERITY_HIGH:
        gl_logger->error("[GL] [{}] {}", t, message);
        break;
      case GL_DEBUG_SEVERITY_NOTIFICATION:
      default:
        // ignore, these are purerly informational
        break;
    }
  }
}
#endif // SHAR_DEBUG_BUILD

namespace shar::ui {

struct SDLDevice {
  struct nk_buffer cmds;
  struct nk_draw_null_texture null;

  GLuint vbo;  // vertex buffer object id
  GLuint vao;  // vertex array object id
  GLuint ebo;  // element (index) buffer object id

  GLuint font_tex;
};

struct Vertex {
  float position[2];
  float uv[2];
  nk_byte color[4];
};

void Renderer::init_log(const Logger &logger) {
#ifdef SHAR_DEBUG_BUILD
  gl_logger = logger;
#else
  (void)logger;
#endif
}

Renderer::Renderer(OpenGLVTable table)
    : m_gl(std::move(table))
    , m_device(std::make_unique<SDLDevice>())
    , m_atlas(std::make_unique<nk_font_atlas>()) {
  init();
}

Renderer::~Renderer() {
  destroy();
}

Shader Renderer::compile(const char* vertex_shader,
                         const char* fragment_shader) {
  Shader shader;

  shader.vertex = m_gl.glCreateShader(GL_VERTEX_SHADER);
  m_gl.glShaderSource(shader.vertex, 1, &vertex_shader, nullptr);
  m_gl.glCompileShader(shader.vertex);

  shader.fragment = m_gl.glCreateShader(GL_FRAGMENT_SHADER);
  m_gl.glShaderSource(shader.fragment, 1, &fragment_shader, nullptr);
  m_gl.glCompileShader(shader.fragment);

  const auto check = [this] (GLint status, ProgramID shader) {
    const bool failed = status != GL_TRUE;

    if (failed) {
      GLint len = 0;
      m_gl.glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
      std::string message(static_cast<usize>(len), ' ');
      m_gl.glGetShaderInfoLog(shader, len, &len, message.data());

      // FIXME: leaks shader
      throw std::runtime_error("Compilation error: " + message);
    }
  };

  GLint status = GL_FALSE;
  m_gl.glGetShaderiv(shader.vertex, GL_COMPILE_STATUS, &status);
  check(status, shader.vertex);

  m_gl.glGetShaderiv(shader.fragment, GL_COMPILE_STATUS, &status);
  check(status, shader.fragment);

  shader.program = m_gl.glCreateProgram();
  m_gl.glAttachShader(shader.program, shader.vertex);
  m_gl.glAttachShader(shader.program, shader.fragment);
  m_gl.glLinkProgram(shader.program);
  m_gl.glGetProgramiv(shader.program, GL_LINK_STATUS, &status);
  assert(status == GL_TRUE);

  shader.attributes.texture = static_cast<AttributeID>(
      m_gl.glGetUniformLocation(shader.program, "Texture")
  );
  shader.attributes.projection = static_cast<AttributeID>(
      m_gl.glGetUniformLocation(shader.program, "Projection")
  );
  shader.attributes.position = static_cast<AttributeID>(
      m_gl.glGetAttribLocation(shader.program, "Position")
  );
  shader.attributes.texture_coordinates = static_cast<AttributeID>(
      m_gl.glGetAttribLocation(shader.program, "TexCoord")
  );
  shader.attributes.color = static_cast<AttributeID>(
      m_gl.glGetAttribLocation(shader.program, "Color")
  );

  return shader;
}

void Renderer::init() {

#ifdef SHAR_DEBUG_BUILD
  // opengl debug output
  if (void *ptr = SDL_GL_GetProcAddress("glDebugMessageCallback")) {
    glEnable(GL_DEBUG_OUTPUT);
    auto debug_output = reinterpret_cast<PFNGLDEBUGMESSAGECALLBACKARBPROC>(ptr);
    debug_output(opengl_error_callback, nullptr);
  }
#endif

  static const GLchar* nk_vertex_shader =
      SHAR_SHADER_VERSION
      "uniform mat4 Projection;\n"
      "in vec2 Position;\n"
      "in vec2 TexCoord;\n"
      "in vec4 Color;\n"
      "out vec2 Frag_UV;\n"
      "out vec4 Frag_Color;\n"
      "void main() {\n"
      "   Frag_UV = TexCoord;\n"
      "   Frag_Color = Color;\n"
      "   gl_Position = Projection * vec4(Position.xy, 0, 1);\n"
      "}\n";

  static const GLchar* nk_fragment_shader =
      SHAR_SHADER_VERSION
      "precision mediump float;\n"
      "uniform sampler2D Texture;\n"
      "in vec2 Frag_UV;\n"
      "in vec4 Frag_Color;\n"
      "out vec4 Out_Color;\n"
      "void main(){\n"
      "   Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
      "}\n";

  nk_buffer_init_default(&m_device->cmds);
  m_shader = compile(nk_vertex_shader, nk_fragment_shader);
  {
    /* buffer setup */
    GLsizei vertex_size = sizeof(Vertex);
    size_t position_offset = offsetof(Vertex, position);
    size_t texture_coordinates_offset  = offsetof(Vertex, uv);
    size_t color_offset = offsetof(Vertex, color);

    m_gl.glGenBuffers(1, &m_device->vbo);
    m_gl.glGenBuffers(1, &m_device->ebo);
    m_gl.glGenVertexArrays(1, &m_device->vao);

    m_gl.glBindVertexArray(m_device->vao);
    m_gl.glBindBuffer(GL_ARRAY_BUFFER, m_device->vbo);
    m_gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_device->ebo);

    m_gl.glEnableVertexAttribArray(m_shader.attributes.position);
    m_gl.glEnableVertexAttribArray(m_shader.attributes.texture_coordinates);
    m_gl.glEnableVertexAttribArray(m_shader.attributes.color);

    m_gl.glVertexAttribPointer(
        m_shader.attributes.position,
        2,
        GL_FLOAT,
        GL_FALSE,
        vertex_size,
        reinterpret_cast<void*>(position_offset)
    );
    m_gl.glVertexAttribPointer(
        m_shader.attributes.texture_coordinates,
        2,
        GL_FLOAT,
        GL_FALSE,
        vertex_size,
        reinterpret_cast<void*>(texture_coordinates_offset)
    );
    m_gl.glVertexAttribPointer(m_shader.attributes.color,
        4,
        GL_UNSIGNED_BYTE,
        GL_TRUE,
        vertex_size,
        reinterpret_cast<void*>(color_offset)
    );
  }

  glBindTexture(GL_TEXTURE_2D, 0);
  m_gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
  m_gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  m_gl.glBindVertexArray(0);

  // init fonts
  nk_font_atlas_init_default(m_atlas.get());
  nk_font_atlas_begin(m_atlas.get());
  int w, h;
  const void *image =
      nk_font_atlas_bake(m_atlas.get(), &w, &h, NK_FONT_ATLAS_RGBA32);

  // nk_sdl_device_upload_atlas(sdl, image, w, h);
  glGenTextures(1, &m_device->font_tex);
  glBindTexture(GL_TEXTURE_2D, m_device->font_tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D,
               0,
               GL_RGBA,
               static_cast<GLsizei>(w),
               static_cast<GLsizei>(h),
               0,
               GL_RGBA,
               GL_UNSIGNED_BYTE,
               image);

  nk_font_atlas_end(m_atlas.get(),
                    nk_handle_id(static_cast<int>(m_device->font_tex)),
                    &m_device->null);
}

void Renderer::free(Shader& shader) {
  if (shader.program != 0) {
    m_gl.glDetachShader(shader.program, shader.vertex);
    m_gl.glDetachShader(shader.program, shader.fragment);
    m_gl.glDeleteProgram(shader.program);
  }

  if (shader.vertex != 0)
    m_gl.glDeleteShader(shader.vertex);

  if (shader.fragment != 0)
    m_gl.glDeleteShader(shader.fragment);
}

void Renderer::destroy() {
  nk_font_atlas_clear(m_atlas.get());

  free(m_shader);

  glDeleteTextures(1, &m_device->font_tex);
  m_gl.glDeleteBuffers(1, &m_device->vbo);
  m_gl.glDeleteBuffers(1, &m_device->ebo);
  nk_buffer_free(&m_device->cmds);
}

void Renderer::render(State &state, const Window &window) {
  float ortho[4][4] = {
      { 2.0f,  0.0f,  0.0f,  0.0f},
      { 0.0f, -2.0f,  0.0f,  0.0f},
      { 0.0f,  0.0f, -1.0f,  0.0f},
      {-1.0f,  1.0f,  0.0f,  1.0f},
  };

  float width = static_cast<float>(window.size().width());
  float height = static_cast<float>(window.size().height());

  ortho[0][0] /= width;
  ortho[1][1] /= height;

  /* setup global state */
  int display_width = static_cast<int>(window.display_size().width());
  int display_height = static_cast<int>(window.display_size().height());

  glViewport(0, 0, display_width, display_height);
  glEnable(GL_BLEND);
  m_gl.glBlendEquation(GL_FUNC_ADD);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_SCISSOR_TEST);
  m_gl.glActiveTexture(GL_TEXTURE0);

  /* setup program */
  m_gl.glUseProgram(m_shader.program);
  m_gl.glUniform1i(m_shader.attributes.texture, 0);
  m_gl.glUniformMatrix4fv(m_shader.attributes.projection, 1, GL_FALSE, &ortho[0][0]);
  {
    /* convert from command queue into draw list and draw to screen */

    /* allocate vertex and element buffer */
    m_gl.glBindVertexArray(m_device->vao);
    m_gl.glBindBuffer(GL_ARRAY_BUFFER, m_device->vbo);
    m_gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_device->ebo);

    m_gl.glBufferData(GL_ARRAY_BUFFER,
                      MAX_VERTEX_MEMORY,
                      nullptr,
                      GL_STREAM_DRAW);
    m_gl.glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                      MAX_ELEMENT_MEMORY,
                      nullptr,
                      GL_STREAM_DRAW);

    /* load vertices/elements directly into vertex/element buffer */
    Vertex* vertices = reinterpret_cast<Vertex*>(m_gl.glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
    u16* elements = reinterpret_cast<u16*>(m_gl.glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY));
    {
      /* fill convert configuration */
      static const struct nk_draw_vertex_layout_element vertex_layout[] = {
          {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, offsetof(Vertex, position)},
          {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, offsetof(Vertex, uv)},
          {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, offsetof(Vertex, color)},
          {NK_VERTEX_LAYOUT_END}};

      struct nk_convert_config config;
      std::memset(&config, 0, sizeof(config));
      config.vertex_layout = vertex_layout;
      config.vertex_size = sizeof(Vertex);
      config.vertex_alignment = alignof(Vertex);
      config.null = m_device->null;
      config.circle_segment_count = 22;
      config.curve_segment_count = 22;
      config.arc_segment_count = 22;
      config.global_alpha = 1.0f;
      config.shape_AA = NK_ANTI_ALIASING_ON;
      config.line_AA = NK_ANTI_ALIASING_ON;

      /* setup buffers to load vertices and elements */
      struct nk_buffer vertex_buffer;
      struct nk_buffer elements_buffer;

      nk_buffer_init_fixed(&vertex_buffer, vertices, MAX_VERTEX_MEMORY);
      nk_buffer_init_fixed(&elements_buffer, elements, MAX_ELEMENT_MEMORY);
      nk_convert(state.context(), &m_device->cmds, &vertex_buffer, &elements_buffer, &config);
    }
    m_gl.glUnmapBuffer(GL_ARRAY_BUFFER);
    m_gl.glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

    struct nk_vec2 scale;
    scale.x = static_cast<float>(display_width) / static_cast<float>(width);
    scale.y = static_cast<float>(display_height) / static_cast<float>(height);

    /* iterate over and execute each draw command */
    auto* ctx = state.context();

    // NOTE: this is not really a pointer
    const nk_draw_index* offset = nullptr;

    const struct nk_draw_command* cmd = nk__draw_begin(ctx, &m_device->cmds);
    for (; cmd != nullptr; cmd = nk__draw_next(cmd, &m_device->cmds, ctx))
    {
      if (cmd->elem_count == 0)
        continue;

      // Make the specified texture active
      glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(cmd->texture.id));

      // Clip the render target to |cmd->clip_rect|
      const auto x   = static_cast<int>(cmd->clip_rect.x * scale.x);
      const auto top = static_cast<int>(cmd->clip_rect.y + cmd->clip_rect.h);
      const auto y   = static_cast<int>((height - top) * scale.y);
      const auto w   = static_cast<int>(cmd->clip_rect.w * scale.x);
      const auto h   = static_cast<int>(cmd->clip_rect.h * scale.y);
      glScissor(x, y, w, h);

      // Execute the command
      glDrawElements(GL_TRIANGLES,
                     static_cast<GLsizei>(cmd->elem_count),
                     GL_UNSIGNED_SHORT,
                     offset);
      offset += cmd->elem_count;
    }
    state.clear();
  }

  m_gl.glUseProgram(0);
  m_gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
  m_gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  m_gl.glBindVertexArray(0);

  glDisable(GL_BLEND);
  glDisable(GL_SCISSOR_TEST);
}

void Renderer::render(Texture& texture,
                      const Window& window,
                      Point at,
                      usize x_offset,
                      usize y_offset) {
  // TODO: extract global setup to a separate function to reuse for UI & video rendering
  float ortho[4][4] = {
      { 2.0f,  0.0f,  0.0f,  0.0f},
      { 0.0f, -2.0f,  0.0f,  0.0f},
      { 0.0f,  0.0f, -1.0f,  0.0f},
      {-1.0f,  1.0f,  0.0f,  1.0f},
  };

  float w = static_cast<float>(window.size().width());
  float h = static_cast<float>(window.size().height());

  ortho[0][0] /= w;
  ortho[1][1] /= h;

  /* setup global state */
  int display_width = static_cast<int>(window.display_size().width());
  int display_height = static_cast<int>(window.display_size().height());

  glViewport(0, 0, display_width, display_height);
  glDisable(GL_BLEND);
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_SCISSOR_TEST);
  m_gl.glActiveTexture(GL_TEXTURE0);

  /* setup program */
  m_gl.glUseProgram(m_shader.program);
  m_gl.glUniform1i(m_shader.attributes.texture, 0);
  m_gl.glUniformMatrix4fv(m_shader.attributes.projection, 1, GL_FALSE, &ortho[0][0]);

  float x  = static_cast<float>(at.x);
  float y  = static_cast<float>(at.y);
  float xo = static_cast<float>(x_offset);
  float yo = static_cast<float>(y_offset);

  Vertex vertices_data[] = {
      //      x       y       u     v      color
      Vertex{ x,      y,      0.0f, 0.0f, {0xff, 0xff, 0xff, 0xff} }, // bottom left corner
      Vertex{ x,      h - yo, 0.0f, 1.0f, {0xff, 0xff, 0xff, 0xff} }, // top left corner
      Vertex{ w - xo, h - yo, 1.0f, 1.0f, {0xff, 0xff, 0xff, 0xff} }, // top right corner
      Vertex{ w - xo, y,      1.0f, 0.0f, {0xff, 0xff, 0xff, 0xff} }, // bottom right corner
  };

  const u16 elements_data[] = {
      0, 2, 1,     // first triangle (bottom left - top left - top right)
      0, 3, 2
  };

   /* allocate vertex and element buffer */
  m_gl.glBindVertexArray(m_device->vao);
  m_gl.glBindBuffer(GL_ARRAY_BUFFER, m_device->vbo);
  m_gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_device->ebo);

  m_gl.glBufferData(GL_ARRAY_BUFFER,
                    MAX_VERTEX_MEMORY,
                    nullptr,
                    GL_STREAM_DRAW);
  m_gl.glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                    MAX_ELEMENT_MEMORY,
                    nullptr,
                    GL_STREAM_DRAW);

  /* load vertices/elements directly into vertex/element buffer */
  void* vertices = m_gl.glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
  void* elements = m_gl.glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);

  std::memcpy(vertices, vertices_data, sizeof(Vertex) * 4);
  std::memcpy(elements, elements_data, sizeof(u16) * 6);

  m_gl.glUnmapBuffer(GL_ARRAY_BUFFER);
  m_gl.glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

  texture.bind();

  void* offset = nullptr;
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, offset);

  texture.unbind();

  m_gl.glUseProgram(0);
  m_gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
  m_gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  m_gl.glBindVertexArray(0);
}

nk_user_font *Renderer::default_font_handle() const {
  return &m_atlas->default_font->handle;
}

} // namespace shar::ui
