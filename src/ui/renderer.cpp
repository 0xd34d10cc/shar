#include "renderer.hpp"

#include <cassert>
#include <cstdlib> // memset

#include "nk.hpp"
#include "gl_vtable.hpp"
#include "window.hpp"
#include "state.hpp"
#include "texture.hpp"


#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

#ifdef __APPLE__
#define SHAR_SHADER_VERSION "#version 150\n"
#else
#define SHAR_SHADER_VERSION "#version 300 es\n"
#endif


namespace shar::ui {

struct SDLDevice {
  struct nk_buffer cmds;
  struct nk_draw_null_texture null;
  GLuint vbo, vao, ebo;
  GLuint prog;
  GLuint vert_shdr;
  GLuint frag_shdr;
  GLint attrib_pos;
  GLint attrib_uv;
  GLint attrib_col;
  GLint uniform_tex;
  GLint uniform_proj;
  GLuint font_tex;
};

struct Vertex {
  float position[2];
  float uv[2];
  nk_byte col[4];
};

Renderer::Renderer(OpenGLVTable table)
  : m_device(std::make_unique<SDLDevice>())
  , m_gl(std::move(table))
  , m_atlas(std::make_unique<nk_font_atlas>())
{
  init();
}

Renderer::~Renderer() {
  destroy();
}

void Renderer::init() {
  GLint status;
  static const GLchar* vertex_shader =
    SHAR_SHADER_VERSION
    "uniform mat4 ProjMtx;\n"
    "in vec2 Position;\n"
    "in vec2 TexCoord;\n"
    "in vec4 Color;\n"
    "out vec2 Frag_UV;\n"
    "out vec4 Frag_Color;\n"
    "void main() {\n"
    "   Frag_UV = TexCoord;\n"
    "   Frag_Color = Color;\n"
    "   gl_Position = ProjMtx * vec4(Position.xy, 0, 1);\n"
    "}\n";

  static const GLchar* fragment_shader =
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
  // NOTE: these are extension functions, so we have to load them separately
  //       either manually or via library (e.g. glew)
  m_device->prog = m_gl.glCreateProgram();
  m_device->vert_shdr = m_gl.glCreateShader(GL_VERTEX_SHADER);
  m_device->frag_shdr = m_gl.glCreateShader(GL_FRAGMENT_SHADER);
  m_gl.glShaderSource(m_device->vert_shdr, 1, &vertex_shader, 0);
  m_gl.glShaderSource(m_device->frag_shdr, 1, &fragment_shader, 0);
  m_gl.glCompileShader(m_device->vert_shdr);
  m_gl.glCompileShader(m_device->frag_shdr);
  m_gl.glGetShaderiv(m_device->vert_shdr, GL_COMPILE_STATUS, &status);
  assert(status == GL_TRUE);
  m_gl.glGetShaderiv(m_device->frag_shdr, GL_COMPILE_STATUS, &status);
  assert(status == GL_TRUE);
  m_gl.glAttachShader(m_device->prog, m_device->vert_shdr);
  m_gl.glAttachShader(m_device->prog, m_device->frag_shdr);
  m_gl.glLinkProgram(m_device->prog);
  m_gl.glGetProgramiv(m_device->prog, GL_LINK_STATUS, &status);
  assert(status == GL_TRUE);

  m_device->uniform_tex = m_gl.glGetUniformLocation(m_device->prog, "Texture");
  m_device->uniform_proj = m_gl.glGetUniformLocation(m_device->prog, "ProjMtx");
  m_device->attrib_pos = m_gl.glGetAttribLocation(m_device->prog, "Position");
  m_device->attrib_uv = m_gl.glGetAttribLocation(m_device->prog, "TexCoord");
  m_device->attrib_col = m_gl.glGetAttribLocation(m_device->prog, "Color");

  {
    /* buffer setup */
    GLsizei vs = sizeof(Vertex);
    size_t vp = offsetof(Vertex, position);
    size_t vt = offsetof(Vertex, uv);
    size_t vc = offsetof(Vertex, col);

    m_gl.glGenBuffers(1, &m_device->vbo);
    m_gl.glGenBuffers(1, &m_device->ebo);
    m_gl.glGenVertexArrays(1, &m_device->vao);

    m_gl.glBindVertexArray(m_device->vao);
    m_gl.glBindBuffer(GL_ARRAY_BUFFER, m_device->vbo);
    m_gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_device->ebo);

    m_gl.glEnableVertexAttribArray((GLuint)m_device->attrib_pos);
    m_gl.glEnableVertexAttribArray((GLuint)m_device->attrib_uv);
    m_gl.glEnableVertexAttribArray((GLuint)m_device->attrib_col);

    m_gl.glVertexAttribPointer((GLuint)m_device->attrib_pos, 2, GL_FLOAT, GL_FALSE, vs, (void*)vp);
    m_gl.glVertexAttribPointer((GLuint)m_device->attrib_uv, 2, GL_FLOAT, GL_FALSE, vs, (void*)vt);
    m_gl.glVertexAttribPointer((GLuint)m_device->attrib_col, 4, GL_UNSIGNED_BYTE, GL_TRUE, vs, (void*)vc);
  }

  glBindTexture(GL_TEXTURE_2D, 0);
  m_gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
  m_gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  m_gl.glBindVertexArray(0);

  // init fonts
  nk_font_atlas_init_default(m_atlas.get());
  nk_font_atlas_begin(m_atlas.get());
  int w, h;
  const void* image = nk_font_atlas_bake(m_atlas.get(), &w, &h, NK_FONT_ATLAS_RGBA32);

  // nk_sdl_device_upload_atlas(sdl, image, w, h);
  glGenTextures(1, &m_device->font_tex);
  glBindTexture(GL_TEXTURE_2D, m_device->font_tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)w, (GLsizei)h, 0,
    GL_RGBA, GL_UNSIGNED_BYTE, image);

  nk_font_atlas_end(m_atlas.get(), nk_handle_id((int)m_device->font_tex), &m_device->null);
}

void Renderer::destroy() {
  nk_font_atlas_clear(m_atlas.get());
  m_gl.glDetachShader(m_device->prog, m_device->vert_shdr);
  m_gl.glDetachShader(m_device->prog, m_device->frag_shdr);
  m_gl.glDeleteShader(m_device->vert_shdr);
  m_gl.glDeleteShader(m_device->frag_shdr);
  m_gl.glDeleteProgram(m_device->prog);
  glDeleteTextures(1, &m_device->font_tex);
  m_gl.glDeleteBuffers(1, &m_device->vbo);
  m_gl.glDeleteBuffers(1, &m_device->ebo);
  nk_buffer_free(&m_device->cmds);
}

void Renderer::render(State& state, const Window& window) {
  GLfloat ortho[4][4] = {
      {2.0f, 0.0f, 0.0f, 0.0f},
      {0.0f,-2.0f, 0.0f, 0.0f},
      {0.0f, 0.0f,-1.0f, 0.0f},
      {-1.0f,1.0f, 0.0f, 1.0f},
  };

  int width = static_cast<int>(window.size().width());
  int height = static_cast<int>(window.size().height());

  int display_width = static_cast<int>(window.display_size().width());
  int display_height = static_cast<int>(window.display_size().height());

  ortho[0][0] /= (GLfloat)width;
  ortho[1][1] /= (GLfloat)height;

  struct nk_vec2 scale;
  scale.x = (float)display_width / (float)width;
  scale.y = (float)display_height / (float)height;

  /* setup global state */
  glViewport(0, 0, display_width, display_height);
  glEnable(GL_BLEND);
  m_gl.glBlendEquation(GL_FUNC_ADD);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_SCISSOR_TEST);
  m_gl.glActiveTexture(GL_TEXTURE0);

  /* setup program */
  m_gl.glUseProgram(m_device->prog);
  m_gl.glUniform1i(m_device->uniform_tex, 0);
  m_gl.glUniformMatrix4fv(m_device->uniform_proj, 1, GL_FALSE, &ortho[0][0]);
  {
    /* convert from command queue into draw list and draw to screen */
    const struct nk_draw_command* cmd;
    void* vertices, * elements;
    const nk_draw_index* offset = NULL;
    struct nk_buffer vbuf, ebuf;

    /* allocate vertex and element buffer */
    m_gl.glBindVertexArray(m_device->vao);
    m_gl.glBindBuffer(GL_ARRAY_BUFFER, m_device->vbo);
    m_gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_device->ebo);

    m_gl.glBufferData(GL_ARRAY_BUFFER, MAX_VERTEX_MEMORY, NULL, GL_STREAM_DRAW);
    m_gl.glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_ELEMENT_MEMORY, NULL, GL_STREAM_DRAW);

    /* load vertices/elements directly into vertex/element buffer */
    vertices = m_gl.glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    elements = m_gl.glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
    {
      /* fill convert configuration */
      struct nk_convert_config config;
      static const struct nk_draw_vertex_layout_element vertex_layout[] = {
          {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(Vertex, position)},
          {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(Vertex, uv)},
          {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(Vertex, col)},
          {NK_VERTEX_LAYOUT_END}
      };
      std::memset(&config, 0, sizeof(config));
      config.vertex_layout = vertex_layout;
      config.vertex_size = sizeof(Vertex);
      config.vertex_alignment = NK_ALIGNOF(Vertex);
      config.null = m_device->null;
      config.circle_segment_count = 22;
      config.curve_segment_count = 22;
      config.arc_segment_count = 22;
      config.global_alpha = 1.0f;
      config.shape_AA = NK_ANTI_ALIASING_ON;
      config.line_AA = NK_ANTI_ALIASING_ON;

      /* setup buffers to load vertices and elements */
      nk_buffer_init_fixed(&vbuf, vertices, (nk_size)MAX_VERTEX_MEMORY);
      nk_buffer_init_fixed(&ebuf, elements, (nk_size)MAX_ELEMENT_MEMORY);
      nk_convert(state.context(), &m_device->cmds, &vbuf, &ebuf, &config);
    }
    m_gl.glUnmapBuffer(GL_ARRAY_BUFFER);
    m_gl.glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

    /* iterate over and execute each draw command */
    nk_draw_foreach(cmd, state.context(), &m_device->cmds) {
      if (cmd->elem_count == 0) continue;

      glBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.id);
      glScissor((GLint)(cmd->clip_rect.x * scale.x),
        (GLint)((height - (GLint)(cmd->clip_rect.y + cmd->clip_rect.h)) * scale.y),
        (GLint)(cmd->clip_rect.w * scale.x),
        (GLint)(cmd->clip_rect.h * scale.y));
      glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, offset);
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

void Renderer::render(Texture& texture) {
  texture.bind();

  // TODO: use shaders
  glBegin(GL_QUADS);

  glTexCoord2f(0, 0);
  glVertex3f(-1, 1, 0);

  glTexCoord2f(0, 1);
  glVertex3f(-1, -1, 0);

  glTexCoord2f(1, 1);
  glVertex3f(1, -1, 0);

  glTexCoord2f(1, 0);
  glVertex3f(1, 1, 0);

  glEnd();

  texture.unbind();
}

nk_user_font* Renderer::default_font_handle() const {
  return &m_atlas->default_font->handle;
}

}