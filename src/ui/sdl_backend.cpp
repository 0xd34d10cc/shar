#include "sdl_backend.hpp"

#include <cstring>

#include "disable_warnings_push.hpp"
#ifdef WIN32
#define GL_BGRA 0x80E1
#include <windows.h>
#endif
#include <GL/gl.h>
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#include <nuklear.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_clipboard.h>
#include <SDL2/SDL_opengl_glext.h>
#include "disable_warnings_pop.hpp"

#include "gl_vtable.hpp"


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

struct nk_sdl_vertex {
  float position[2];
  float uv[2];
  nk_byte col[4];
};

struct SDLBackend {
  SDL_Window* win;
  struct SDLDevice ogl;
  struct nk_context ctx;
  struct nk_font_atlas atlas;
};

#ifdef __APPLE__
#define NK_SHADER_VERSION "#version 150\n"
#else
#define NK_SHADER_VERSION "#version 300 es\n"
#endif

static void nk_sdl_device_create(SDLBackend* sdl, OpenGLVTable* gl) {
  GLint status;
  static const GLchar* vertex_shader =
    NK_SHADER_VERSION
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
    NK_SHADER_VERSION
    "precision mediump float;\n"
    "uniform sampler2D Texture;\n"
    "in vec2 Frag_UV;\n"
    "in vec4 Frag_Color;\n"
    "out vec4 Out_Color;\n"
    "void main(){\n"
    "   Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
    "}\n";

  struct SDLDevice* dev = &sdl->ogl;
  nk_buffer_init_default(&dev->cmds);
  // NOTE: these are extension functions, so we have to load them separately
  //       either manually or via library (e.g. glew)
  dev->prog = gl->glCreateProgram();
  dev->vert_shdr = gl->glCreateShader(GL_VERTEX_SHADER);
  dev->frag_shdr = gl->glCreateShader(GL_FRAGMENT_SHADER);
  gl->glShaderSource(dev->vert_shdr, 1, &vertex_shader, 0);
  gl->glShaderSource(dev->frag_shdr, 1, &fragment_shader, 0);
  gl->glCompileShader(dev->vert_shdr);
  gl->glCompileShader(dev->frag_shdr);
  gl->glGetShaderiv(dev->vert_shdr, GL_COMPILE_STATUS, &status);
  assert(status == GL_TRUE);
  gl->glGetShaderiv(dev->frag_shdr, GL_COMPILE_STATUS, &status);
  assert(status == GL_TRUE);
  gl->glAttachShader(dev->prog, dev->vert_shdr);
  gl->glAttachShader(dev->prog, dev->frag_shdr);
  gl->glLinkProgram(dev->prog);
  gl->glGetProgramiv(dev->prog, GL_LINK_STATUS, &status);
  assert(status == GL_TRUE);

  dev->uniform_tex = gl->glGetUniformLocation(dev->prog, "Texture");
  dev->uniform_proj = gl->glGetUniformLocation(dev->prog, "ProjMtx");
  dev->attrib_pos = gl->glGetAttribLocation(dev->prog, "Position");
  dev->attrib_uv = gl->glGetAttribLocation(dev->prog, "TexCoord");
  dev->attrib_col = gl->glGetAttribLocation(dev->prog, "Color");

  {
    /* buffer setup */
    GLsizei vs = sizeof(struct nk_sdl_vertex);
    size_t vp = offsetof(struct nk_sdl_vertex, position);
    size_t vt = offsetof(struct nk_sdl_vertex, uv);
    size_t vc = offsetof(struct nk_sdl_vertex, col);

    gl->glGenBuffers(1, &dev->vbo);
    gl->glGenBuffers(1, &dev->ebo);
    gl->glGenVertexArrays(1, &dev->vao);

    gl->glBindVertexArray(dev->vao);
    gl->glBindBuffer(GL_ARRAY_BUFFER, dev->vbo);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dev->ebo);

    gl->glEnableVertexAttribArray((GLuint)dev->attrib_pos);
    gl->glEnableVertexAttribArray((GLuint)dev->attrib_uv);
    gl->glEnableVertexAttribArray((GLuint)dev->attrib_col);

    gl->glVertexAttribPointer((GLuint)dev->attrib_pos, 2, GL_FLOAT, GL_FALSE, vs, (void*)vp);
    gl->glVertexAttribPointer((GLuint)dev->attrib_uv, 2, GL_FLOAT, GL_FALSE, vs, (void*)vt);
    gl->glVertexAttribPointer((GLuint)dev->attrib_col, 4, GL_UNSIGNED_BYTE, GL_TRUE, vs, (void*)vc);
  }

  glBindTexture(GL_TEXTURE_2D, 0);
  gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
  gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  gl->glBindVertexArray(0);
}

NK_INTERN void
nk_sdl_device_upload_atlas(SDLBackend* sdl, const void* image, int width, int height)
{
  struct SDLDevice* dev = &sdl->ogl;
  glGenTextures(1, &dev->font_tex);
  glBindTexture(GL_TEXTURE_2D, dev->font_tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0,
    GL_RGBA, GL_UNSIGNED_BYTE, image);
}

static void nk_sdl_device_destroy(SDLBackend* sdl, OpenGLVTable* gl)
{
  struct SDLDevice* dev = &sdl->ogl;
  gl->glDetachShader(dev->prog, dev->vert_shdr);
  gl->glDetachShader(dev->prog, dev->frag_shdr);
  gl->glDeleteShader(dev->vert_shdr);
  gl->glDeleteShader(dev->frag_shdr);
  gl->glDeleteProgram(dev->prog);
  glDeleteTextures(1, &dev->font_tex);
  gl->glDeleteBuffers(1, &dev->vbo);
  gl->glDeleteBuffers(1, &dev->ebo);
  nk_buffer_free(&dev->cmds);
}

NK_API void
nk_sdl_render(SDLBackend* sdl,
              OpenGLVTable* gl,
              enum nk_anti_aliasing AA,
              int max_vertex_buffer,
              int max_element_buffer)
{
  struct SDLDevice* dev = &sdl->ogl;
  int width, height;
  int display_width, display_height;
  struct nk_vec2 scale;
  GLfloat ortho[4][4] = {
      {2.0f, 0.0f, 0.0f, 0.0f},
      {0.0f,-2.0f, 0.0f, 0.0f},
      {0.0f, 0.0f,-1.0f, 0.0f},
      {-1.0f,1.0f, 0.0f, 1.0f},
  };
  SDL_GetWindowSize(sdl->win, &width, &height);
  SDL_GL_GetDrawableSize(sdl->win, &display_width, &display_height);
  ortho[0][0] /= (GLfloat)width;
  ortho[1][1] /= (GLfloat)height;

  scale.x = (float)display_width / (float)width;
  scale.y = (float)display_height / (float)height;

  /* setup global state */
  glViewport(0, 0, display_width, display_height);
  glEnable(GL_BLEND);
  gl->glBlendEquation(GL_FUNC_ADD);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_SCISSOR_TEST);
  gl->glActiveTexture(GL_TEXTURE0);

  /* setup program */
  gl->glUseProgram(dev->prog);
  gl->glUniform1i(dev->uniform_tex, 0);
  gl->glUniformMatrix4fv(dev->uniform_proj, 1, GL_FALSE, &ortho[0][0]);
  {
    /* convert from command queue into draw list and draw to screen */
    const struct nk_draw_command* cmd;
    void* vertices, * elements;
    const nk_draw_index* offset = NULL;
    struct nk_buffer vbuf, ebuf;

    /* allocate vertex and element buffer */
    gl->glBindVertexArray(dev->vao);
    gl->glBindBuffer(GL_ARRAY_BUFFER, dev->vbo);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dev->ebo);

    gl->glBufferData(GL_ARRAY_BUFFER, max_vertex_buffer, NULL, GL_STREAM_DRAW);
    gl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, max_element_buffer, NULL, GL_STREAM_DRAW);

    /* load vertices/elements directly into vertex/element buffer */
    vertices = gl->glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    elements = gl->glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
    {
      /* fill convert configuration */
      struct nk_convert_config config;
      static const struct nk_draw_vertex_layout_element vertex_layout[] = {
          {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_sdl_vertex, position)},
          {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_sdl_vertex, uv)},
          {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(struct nk_sdl_vertex, col)},
          {NK_VERTEX_LAYOUT_END}
      };
      NK_MEMSET(&config, 0, sizeof(config));
      config.vertex_layout = vertex_layout;
      config.vertex_size = sizeof(struct nk_sdl_vertex);
      config.vertex_alignment = NK_ALIGNOF(struct nk_sdl_vertex);
      config.null = dev->null;
      config.circle_segment_count = 22;
      config.curve_segment_count = 22;
      config.arc_segment_count = 22;
      config.global_alpha = 1.0f;
      config.shape_AA = AA;
      config.line_AA = AA;

      /* setup buffers to load vertices and elements */
      nk_buffer_init_fixed(&vbuf, vertices, (nk_size)max_vertex_buffer);
      nk_buffer_init_fixed(&ebuf, elements, (nk_size)max_element_buffer);
      nk_convert(&sdl->ctx, &dev->cmds, &vbuf, &ebuf, &config);
    }
    gl->glUnmapBuffer(GL_ARRAY_BUFFER);
    gl->glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

    /* iterate over and execute each draw command */
    nk_draw_foreach(cmd, &sdl->ctx, &dev->cmds) {
      if (!cmd->elem_count) continue;
      glBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.id);
      glScissor((GLint)(cmd->clip_rect.x * scale.x),
        (GLint)((height - (GLint)(cmd->clip_rect.y + cmd->clip_rect.h)) * scale.y),
        (GLint)(cmd->clip_rect.w * scale.x),
        (GLint)(cmd->clip_rect.h * scale.y));
      glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, offset);
      offset += cmd->elem_count;
    }
    nk_clear(&sdl->ctx);
  }

  gl->glUseProgram(0);
  gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
  gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  gl->glBindVertexArray(0);
  glDisable(GL_BLEND);
  glDisable(GL_SCISSOR_TEST);
}

static void
nk_sdl_clipboard_paste(nk_handle usr, struct nk_text_edit* edit)
{
  const char* text = SDL_GetClipboardText();
  if (text) nk_textedit_paste(edit, text, nk_strlen(text));
  (void)usr;
}

static void
nk_sdl_clipboard_copy(nk_handle usr, const char* text, int len)
{
  char* str = 0;
  (void)usr;
  if (!len) return;
  str = (char*)malloc((size_t)len + 1);
  if (!str) return;
  memcpy(str, text, (size_t)len);
  str[len] = '\0';
  SDL_SetClipboardText(str);
  free(str);
}


NK_API struct SDLBackend*
nk_sdl_init(SDL_Window* win, OpenGLVTable* gl)
{
  SDLBackend* sdl = (SDLBackend*)calloc(1, sizeof(SDLBackend));
  sdl->win = win;
  nk_init_default(&sdl->ctx, 0);
  sdl->ctx.clip.copy = nk_sdl_clipboard_copy;
  sdl->ctx.clip.paste = nk_sdl_clipboard_paste;
  sdl->ctx.clip.userdata = nk_handle_ptr(0);
  nk_sdl_device_create(sdl, gl);
  return sdl;
}

NK_API struct nk_context*
nk_sdl_context(SDLBackend* sdl)
{
  return &sdl->ctx;
}

NK_API void
nk_sdl_font_stash_begin(SDLBackend* sdl, struct nk_font_atlas** atlas)
{
  nk_font_atlas_init_default(&sdl->atlas);
  nk_font_atlas_begin(&sdl->atlas);
  *atlas = &sdl->atlas;
}

NK_API void
nk_sdl_font_stash_end(SDLBackend* sdl)
{
  int w, h;
  const void* image = nk_font_atlas_bake(&sdl->atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
  nk_sdl_device_upload_atlas(sdl, image, w, h);
  nk_font_atlas_end(&sdl->atlas, nk_handle_id((int)sdl->ogl.font_tex), &sdl->ogl.null);
  if (sdl->atlas.default_font)
    nk_style_set_font(&sdl->ctx, &sdl->atlas.default_font->handle);

}

NK_API int
nk_sdl_handle_event(SDLBackend* sdl, SDL_Event* evt)
{
  struct nk_context* ctx = &sdl->ctx;
  if (evt->type == SDL_KEYUP || evt->type == SDL_KEYDOWN) {
    /* key events */
    int down = evt->type == SDL_KEYDOWN;
    const Uint8* state = SDL_GetKeyboardState(0);
    SDL_Keycode sym = evt->key.keysym.sym;
    if (sym == SDLK_RSHIFT || sym == SDLK_LSHIFT)
      nk_input_key(ctx, NK_KEY_SHIFT, down);
    else if (sym == SDLK_DELETE)
      nk_input_key(ctx, NK_KEY_DEL, down);
    else if (sym == SDLK_RETURN)
      nk_input_key(ctx, NK_KEY_ENTER, down);
    else if (sym == SDLK_TAB)
      nk_input_key(ctx, NK_KEY_TAB, down);
    else if (sym == SDLK_BACKSPACE)
      nk_input_key(ctx, NK_KEY_BACKSPACE, down);
    else if (sym == SDLK_HOME) {
      nk_input_key(ctx, NK_KEY_TEXT_START, down);
      nk_input_key(ctx, NK_KEY_SCROLL_START, down);
    }
    else if (sym == SDLK_END) {
      nk_input_key(ctx, NK_KEY_TEXT_END, down);
      nk_input_key(ctx, NK_KEY_SCROLL_END, down);
    }
    else if (sym == SDLK_PAGEDOWN) {
      nk_input_key(ctx, NK_KEY_SCROLL_DOWN, down);
    }
    else if (sym == SDLK_PAGEUP) {
      nk_input_key(ctx, NK_KEY_SCROLL_UP, down);
    }
    else if (sym == SDLK_z)
      nk_input_key(ctx, NK_KEY_TEXT_UNDO, down && state[SDL_SCANCODE_LCTRL]);
    else if (sym == SDLK_r)
      nk_input_key(ctx, NK_KEY_TEXT_REDO, down && state[SDL_SCANCODE_LCTRL]);
    else if (sym == SDLK_c)
      nk_input_key(ctx, NK_KEY_COPY, down && state[SDL_SCANCODE_LCTRL]);
    else if (sym == SDLK_v)
      nk_input_key(ctx, NK_KEY_PASTE, down && state[SDL_SCANCODE_LCTRL]);
    else if (sym == SDLK_x)
      nk_input_key(ctx, NK_KEY_CUT, down && state[SDL_SCANCODE_LCTRL]);
    else if (sym == SDLK_b)
      nk_input_key(ctx, NK_KEY_TEXT_LINE_START, down && state[SDL_SCANCODE_LCTRL]);
    else if (sym == SDLK_e)
      nk_input_key(ctx, NK_KEY_TEXT_LINE_END, down && state[SDL_SCANCODE_LCTRL]);
    else if (sym == SDLK_UP)
      nk_input_key(ctx, NK_KEY_UP, down);
    else if (sym == SDLK_DOWN)
      nk_input_key(ctx, NK_KEY_DOWN, down);
    else if (sym == SDLK_LEFT) {
      if (state[SDL_SCANCODE_LCTRL])
        nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, down);
      else nk_input_key(ctx, NK_KEY_LEFT, down);
    }
    else if (sym == SDLK_RIGHT) {
      if (state[SDL_SCANCODE_LCTRL])
        nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, down);
      else nk_input_key(ctx, NK_KEY_RIGHT, down);
    }
    else return 0;
    return 1;
  }
  else if (evt->type == SDL_MOUSEBUTTONDOWN || evt->type == SDL_MOUSEBUTTONUP) {
    /* mouse button */
    int down = evt->type == SDL_MOUSEBUTTONDOWN;
    const int x = evt->button.x, y = evt->button.y;
    if (evt->button.button == SDL_BUTTON_LEFT) {
      if (evt->button.clicks > 1)
        nk_input_button(ctx, NK_BUTTON_LEFT /*NK_BUTTON_DOUBLE*/, x, y, down);
      nk_input_button(ctx, NK_BUTTON_LEFT, x, y, down);
    }
    else if (evt->button.button == SDL_BUTTON_MIDDLE)
      nk_input_button(ctx, NK_BUTTON_MIDDLE, x, y, down);
    else if (evt->button.button == SDL_BUTTON_RIGHT)
      nk_input_button(ctx, NK_BUTTON_RIGHT, x, y, down);
    return 1;
  }
  else if (evt->type == SDL_MOUSEMOTION) {
    /* mouse motion */
    if (ctx->input.mouse.grabbed) {
      int x = (int)ctx->input.mouse.prev.x, y = (int)ctx->input.mouse.prev.y;
      nk_input_motion(ctx, x + evt->motion.xrel, y + evt->motion.yrel);
    }
    else nk_input_motion(ctx, evt->motion.x, evt->motion.y);
    return 1;
  }
  else if (evt->type == SDL_TEXTINPUT) {
    /* text input */
    nk_glyph glyph;
    memcpy(glyph, evt->text.text, NK_UTF_SIZE);
    nk_input_glyph(ctx, glyph);
    return 1;
  }
  else if (evt->type == SDL_MOUSEWHEEL) {
    /* mouse wheel */
    //nk_vec2((float)evt->wheel.x, (float)evt->wheel.y));
    nk_input_scroll(ctx, (float)evt->wheel.y);
    return 1;
  }
  return 0;
}

NK_API
void nk_sdl_shutdown(SDLBackend* sdl, OpenGLVTable* gl)
{
  nk_font_atlas_clear(&sdl->atlas);
  nk_free(&sdl->ctx);
  nk_sdl_device_destroy(sdl, gl);
  free(sdl);
}

} // namespace shar::ui