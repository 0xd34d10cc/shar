#include "state.hpp"

#include "disable_warnings_push.hpp"
#include <SDL2/SDL_clipboard.h>
#include <SDL2/SDL_events.h>
#include "disable_warnings_pop.hpp"

#include "nk.hpp"


namespace shar::ui {

static void clipboard_paste(nk_handle usr, struct nk_text_edit* edit)
{
  const char* text = SDL_GetClipboardText();
  if (text) nk_textedit_paste(edit, text, nk_strlen(text));
  (void)usr;
}

static void clipboard_copy(nk_handle usr, const char* text, int len)
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

State::State()
  : m_context(std::make_unique<nk_context>())
{
  nk_init_default(m_context.get(), 0);
  m_context->clip.copy = clipboard_copy;
  m_context->clip.paste = clipboard_paste;
  m_context->clip.userdata = nk_handle_ptr(0);
}

State::~State() {
  if (m_context)
    nk_free(m_context.get());
}

const nk_context* State::context() const {
  return m_context.get();
}

nk_context* State::context() {
  return m_context.get();
}

bool State::handle(SDL_Event* event) {
  if (event->type == SDL_KEYUP || event->type == SDL_KEYDOWN) {
    /* key events */
    int down = event->type == SDL_KEYDOWN;
    const Uint8* state = SDL_GetKeyboardState(0);
    SDL_Keycode sym = event->key.keysym.sym;
    if (sym == SDLK_RSHIFT || sym == SDLK_LSHIFT)
      nk_input_key(m_context.get(), NK_KEY_SHIFT, down);
    else if (sym == SDLK_DELETE)
      nk_input_key(m_context.get(), NK_KEY_DEL, down);
    else if (sym == SDLK_RETURN)
      nk_input_key(m_context.get(), NK_KEY_ENTER, down);
    else if (sym == SDLK_TAB)
      nk_input_key(m_context.get(), NK_KEY_TAB, down);
    else if (sym == SDLK_BACKSPACE)
      nk_input_key(m_context.get(), NK_KEY_BACKSPACE, down);
    else if (sym == SDLK_HOME) {
      nk_input_key(m_context.get(), NK_KEY_TEXT_START, down);
      nk_input_key(m_context.get(), NK_KEY_SCROLL_START, down);
    }
    else if (sym == SDLK_END) {
      nk_input_key(m_context.get(), NK_KEY_TEXT_END, down);
      nk_input_key(m_context.get(), NK_KEY_SCROLL_END, down);
    }
    else if (sym == SDLK_PAGEDOWN) {
      nk_input_key(m_context.get(), NK_KEY_SCROLL_DOWN, down);
    }
    else if (sym == SDLK_PAGEUP) {
      nk_input_key(m_context.get(), NK_KEY_SCROLL_UP, down);
    }
    else if (sym == SDLK_z)
      nk_input_key(m_context.get(), NK_KEY_TEXT_UNDO, down && state[SDL_SCANCODE_LCTRL]);
    else if (sym == SDLK_r)
      nk_input_key(m_context.get(), NK_KEY_TEXT_REDO, down && state[SDL_SCANCODE_LCTRL]);
    else if (sym == SDLK_c)
      nk_input_key(m_context.get(), NK_KEY_COPY, down && state[SDL_SCANCODE_LCTRL]);
    else if (sym == SDLK_v)
      nk_input_key(m_context.get(), NK_KEY_PASTE, down && state[SDL_SCANCODE_LCTRL]);
    else if (sym == SDLK_x)
      nk_input_key(m_context.get(), NK_KEY_CUT, down && state[SDL_SCANCODE_LCTRL]);
    else if (sym == SDLK_b)
      nk_input_key(m_context.get(), NK_KEY_TEXT_LINE_START, down && state[SDL_SCANCODE_LCTRL]);
    else if (sym == SDLK_e)
      nk_input_key(m_context.get(), NK_KEY_TEXT_LINE_END, down && state[SDL_SCANCODE_LCTRL]);
    else if (sym == SDLK_UP)
      nk_input_key(m_context.get(), NK_KEY_UP, down);
    else if (sym == SDLK_DOWN)
      nk_input_key(m_context.get(), NK_KEY_DOWN, down);
    else if (sym == SDLK_LEFT) {
      if (state[SDL_SCANCODE_LCTRL])
        nk_input_key(m_context.get(), NK_KEY_TEXT_WORD_LEFT, down);
      else nk_input_key(m_context.get(), NK_KEY_LEFT, down);
    }
    else if (sym == SDLK_RIGHT) {
      if (state[SDL_SCANCODE_LCTRL])
        nk_input_key(m_context.get(), NK_KEY_TEXT_WORD_RIGHT, down);
      else nk_input_key(m_context.get(), NK_KEY_RIGHT, down);
    }
    else return false;

    return true;
  }
  else if (event->type == SDL_MOUSEBUTTONDOWN || event->type == SDL_MOUSEBUTTONUP) {
    /* mouse button */
    int down = event->type == SDL_MOUSEBUTTONDOWN;
    const int x = event->button.x;
    const int y = event->button.y;

    if (event->button.button == SDL_BUTTON_LEFT) {
      if (event->button.clicks > 1)
        nk_input_button(m_context.get(), NK_BUTTON_LEFT /*NK_BUTTON_DOUBLE*/, x, y, down);
      nk_input_button(m_context.get(), NK_BUTTON_LEFT, x, y, down);
    }
    else if (event->button.button == SDL_BUTTON_MIDDLE)
      nk_input_button(m_context.get(), NK_BUTTON_MIDDLE, x, y, down);
    else if (event->button.button == SDL_BUTTON_RIGHT)
      nk_input_button(m_context.get(), NK_BUTTON_RIGHT, x, y, down);

    return true;
  }
  else if (event->type == SDL_MOUSEMOTION) {
    /* mouse motion */
    if (m_context.get()->input.mouse.grabbed) {
      int x = (int)m_context.get()->input.mouse.prev.x;
      int y = (int)m_context.get()->input.mouse.prev.y;
      nk_input_motion(m_context.get(), x + event->motion.xrel, y + event->motion.yrel);
    }
    else nk_input_motion(m_context.get(), event->motion.x, event->motion.y);

    return true;
  }
  else if (event->type == SDL_TEXTINPUT) {
    /* text input */
    nk_glyph glyph;
    memcpy(glyph, event->text.text, NK_UTF_SIZE);
    nk_input_glyph(m_context.get(), glyph);
    return true;
  }
  else if (event->type == SDL_MOUSEWHEEL) {
    /* mouse wheel */
    //nk_vec2((float)evt->wheel.x, (float)evt->wheel.y));
    nk_input_scroll(m_context.get(), (float)event->wheel.y);
    return true;
  }

  return false;
}

void State::clear() {
  nk_clear(m_context.get());
}

}