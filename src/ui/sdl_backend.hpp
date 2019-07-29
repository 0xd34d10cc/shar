#pragma once

struct nk_context;
struct SDL_Window;
union SDL_Event;
struct nk_font_atlas;
enum nk_anti_aliasing;

namespace shar::ui {
struct SDLBackend;
struct OpenGLVTable;

struct SDLBackend* nk_sdl_init(SDL_Window* win, OpenGLVTable* gl);
struct nk_context* nk_sdl_context(SDLBackend* sdl);
void nk_sdl_font_stash_begin(SDLBackend* sdl, struct nk_font_atlas** atlas);
void nk_sdl_font_stash_end(SDLBackend* sdl);
int nk_sdl_handle_event(SDLBackend* sdl, SDL_Event* evt);
void nk_sdl_render(SDLBackend* sdl,
                   OpenGLVTable* gl,
                   enum nk_anti_aliasing,
                   int max_vertex_buffer,
                   int max_element_buffer);

void nk_sdl_shutdown(SDLBackend* sdl, OpenGLVTable* gl);

} // namespace shar::ui