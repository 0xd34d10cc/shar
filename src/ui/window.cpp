#include "window.hpp"

#include <cstdio> // printf

#include "disable_warnings_push.hpp"
//#define NK_INCLUDE_FIXED_TYPES
//#define NK_INCLUDE_STANDARD_IO
//#define NK_INCLUDE_STANDARD_VARARGS
//#define NK_INCLUDE_DEFAULT_ALLOCATOR
//#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
//#define NK_INCLUDE_FONT_BAKING
//#define NK_INCLUDE_DEFAULT_FONT
#include <nuklear.h>
#include <SDL2/SDL.h>
#include "disable_warnings_pop.hpp"

#include "sdl_backend.hpp"
#include "gl_vtable.hpp"


namespace shar::ui {

// FIXME
#define WINDOW_WIDTH 1600
#define WINDOW_HEIGHT 900
#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

int run() {
  /* Platform */
  SDL_Window* win;
  SDL_GLContext glContext;
  int win_width, win_height;
  int running = 1;
  SDLBackend* backend;

  /* GUI */
  struct nk_context* ctx;
  struct nk_color bg;
  bg.r = 25,
  bg.g = 25,
  bg.b = 25,
  bg.a = 255;

  /* SDL setup */
  SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // diff
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  win = SDL_CreateWindow("Demo",
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    WINDOW_WIDTH, WINDOW_HEIGHT,
    SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI
  );
  glContext = SDL_GL_CreateContext(win);
  SDL_GetWindowSize(win, &win_width, &win_height);

  /* OpenGL setup */
  glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

  auto glFunctions = OpenGLVTable::load();
  if (!glFunctions) {
    printf("Failed to load OpenGL functions");
    return -1;
  }

  backend = nk_sdl_init(win, &*glFunctions);
  ctx = nk_sdl_context(backend);

  /* Load Fonts: if none of these are loaded a default font will be used  */
  /* Load Cursor: if you uncomment cursor loading please hide the cursor */
  struct nk_font_atlas* atlas;
  nk_sdl_font_stash_begin(backend, &atlas);
  /*struct nk_font *droid = nk_font_atlas_add_from_file(atlas, "../../../extra_font/DroidSans.ttf", 14, 0);*/
  /*struct nk_font *roboto = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Roboto-Regular.ttf", 16, 0);*/
  /*struct nk_font *future = nk_font_atlas_add_from_file(atlas, "../../../extra_font/kenvector_future_thin.ttf", 13, 0);*/
  /*struct nk_font *clean = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyClean.ttf", 12, 0);*/
  /*struct nk_font *tiny = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyTiny.ttf", 10, 0);*/
  /*struct nk_font *cousine = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Cousine-Regular.ttf", 13, 0);*/
  nk_sdl_font_stash_end(backend);

  while (running)
  {
    /* Input */
    SDL_Event evt;
    nk_input_begin(ctx);
    while (SDL_PollEvent(&evt)) {
      if (evt.type == SDL_QUIT) goto cleanup;
      nk_sdl_handle_event(backend, &evt);
    }
    nk_input_end(ctx);

    /* GUI */
    if (nk_begin(ctx, "Demo", nk_rect(50, 50, 230, 250),
                 NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE |
                 NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
    {
      enum { EASY, HARD };
      static int op = EASY;
      static int property = 20;

      nk_layout_row_static(ctx, 30, 80, 1);
      if (nk_button_label(ctx, "button"))
        printf("button pressed!\n");

      nk_layout_row_dynamic(ctx, 30, 2);
      if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
      if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;

      nk_layout_row_dynamic(ctx, 22, 1);
      nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);

      nk_layout_row_dynamic(ctx, 20, 1);
      nk_label(ctx, "background:", NK_TEXT_LEFT);

      nk_layout_row_dynamic(ctx, 25, 1);
      if (nk_combo_begin_color(ctx, bg, nk_vec2(nk_widget_width(ctx), 400))) {
        nk_layout_row_dynamic(ctx, 120, 1);
        bg = nk_color_picker(ctx, bg, NK_RGBA);

        nk_layout_row_dynamic(ctx, 25, 1);

        // FIXME: assigns float to nk_byte
        bg.r = 255.0f * nk_propertyf(ctx, "#R:", 0, (float)bg.r / 255.0f, 1.0f, 0.01f, 0.005f);
        bg.g = 255.0f * nk_propertyf(ctx, "#G:", 0, (float)bg.g / 255.0f, 1.0f, 0.01f, 0.005f);
        bg.b = 255.0f * nk_propertyf(ctx, "#B:", 0, (float)bg.b / 255.0f, 1.0f, 0.01f, 0.005f);
        bg.a = 255.0f * nk_propertyf(ctx, "#A:", 0, (float)bg.a / 255.0f, 1.0f, 0.01f, 0.005f);
        nk_combo_end(ctx);
      }
    }
    nk_end(ctx);

    /* Draw */
    SDL_GetWindowSize(win, &win_width, &win_height);
    glViewport(0, 0, win_width, win_height);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor((float)bg.r / 255.0f, (float)bg.g / 255.0f, (float)bg.b / 255.0f, (float)bg.a / 255.0f);

    /* IMPORTANT: `nk_sdl_render` modifies some global OpenGL state
     * with blending, scissor, face culling, depth test and viewport and
     * defaults everything back into a default state.
     * Make sure to either a.) save and restore or b.) reset your own state after
     * rendering the UI. */
    nk_sdl_render(backend, &*glFunctions, NK_ANTI_ALIASING_ON,
                  MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY);


    SDL_GL_SwapWindow(win);
  }

cleanup:
  nk_sdl_shutdown(backend, &*glFunctions);

  SDL_GL_DeleteContext(glContext);
  SDL_DestroyWindow(win);

  SDL_Quit();
  return 0;
}

}
