#include "text_edit.hpp"

#include "state.hpp"

#include <cstring> // memset

namespace shar::ui {

static unsigned int get_flags(bool read_only, bool multiline, bool copyable) {
  nk_flags flags = NK_EDIT_DEFAULT;
  if (read_only) {
    flags |= NK_EDIT_READ_ONLY | NK_EDIT_NO_CURSOR;
  } else {
    flags |= NK_EDIT_ALWAYS_INSERT_MODE;
  }

  if (multiline) {
    flags |= NK_EDIT_MULTILINE;
  }

  if (copyable) {
    flags |= NK_EDIT_FIELD | NK_EDIT_AUTO_SELECT;
  }
  return flags;
}

TextEdit::TextEdit(bool read_only, bool multiline, bool copyable)
    : m_flags(get_flags(read_only, multiline, copyable)) {
  std::memset(&m_inner, 0, sizeof(nk_text_edit));
  nk_textedit_init_default(&m_inner);
}

TextEdit::~TextEdit() {
  nk_textedit_free(&m_inner);
}

std::string TextEdit::text() const {
  const char *begin = reinterpret_cast<const char *>(
      nk_buffer_memory_const(&m_inner.string.buffer));
  const char *end = begin + m_inner.string.len;
  return std::string{begin, end};
}

void TextEdit::process(State &state) {
  nk_edit_buffer(state.context(), m_flags, &m_inner, nk_filter_default);
}

void TextEdit::set_text(const std::string &text) {
  nk_str_clear(&m_inner.string);
  nk_str_append_str_char(&m_inner.string, text.c_str());
}

} // namespace shar::ui
