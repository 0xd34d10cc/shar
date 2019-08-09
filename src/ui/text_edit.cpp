#include "text_edit.hpp"

#include <cstring> // memset

#include "state.hpp"


namespace shar::ui {

TextEdit::TextEdit() {
  std::memset(&m_inner, 0, sizeof(nk_text_edit));
  nk_textedit_init_default(&m_inner);
}

TextEdit::~TextEdit() {
  nk_textedit_free(&m_inner);
}

std::string TextEdit::text() const {
  const char* begin = (const char*)nk_buffer_memory_const(&m_inner.string.buffer);
  const char* end = begin + m_inner.string.len;
  return std::string{ begin, end };
}

void TextEdit::process(State& state) {
  nk_edit_buffer(state.context(),
                 NK_EDIT_DEFAULT | NK_EDIT_ALWAYS_INSERT_MODE,
                 &m_inner, nk_filter_default);
}

}