/*
 * Nuklear - 1.32.0 - public domain
 * no warrenty implied; use at your own risk.
 * authored from 2015-2016 by Micha Mettke
 */
 /*
  * ==============================================================
  *
  *                              API
  *
  * ===============================================================
  */
#ifndef NK_SDL_GL3_H_
#define NK_SDL_GL3_H_

struct nk_context;
struct SDL_Window;
union SDL_Event;
struct nk_font_atlas;
enum nk_anti_aliasing;
struct OpenGLContext;

OpenGLContext* opengl_load();
void opengl_unload(OpenGLContext* gl);

struct nk_context* nk_sdl_init(SDL_Window* win, OpenGLContext* gl);
void nk_sdl_font_stash_begin(struct nk_font_atlas** atlas);
void nk_sdl_font_stash_end(void);
int nk_sdl_handle_event(SDL_Event* evt);
void nk_sdl_render(OpenGLContext* gl,
                   enum nk_anti_aliasing,
                   int max_vertex_buffer,
                   int max_element_buffer);

void nk_sdl_shutdown(OpenGLContext* gl);
void nk_sdl_device_destroy(OpenGLContext* gl);
void nk_sdl_device_create(OpenGLContext* gl);

#endif