#include "message_box.hpp"

#include <cassert>

#include "disable_warnings_push.hpp"
#include <SDL2/SDL_messagebox.h>
#include "disable_warnings_pop.hpp"


namespace shar::ui {

MessageBox::MessageBox(Type type, std::string title, std::string message)
    : m_type(type)
    , m_title(std::move(title))
    , m_message(std::move(message)) {}

bool MessageBox::show() {
  const auto type_to_sdl = [](Type type) {
    switch (type) {
      case Type::Error:
        return SDL_MESSAGEBOX_ERROR;
      case Type::Warning:
        return SDL_MESSAGEBOX_WARNING;
      case Type::Info:
        return SDL_MESSAGEBOX_INFORMATION;
      default:
        assert(false);
        return SDL_MESSAGEBOX_ERROR;
    }
  };

  int code = SDL_ShowSimpleMessageBox(type_to_sdl(m_type),
                                      m_title.c_str(),
                                      m_message.c_str(),
                                      nullptr);
  return code == 0;
}

} // namespace shar::ui