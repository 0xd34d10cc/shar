#pragma once

#include "ui/nk.hpp"

#include <string_view>


namespace shar::ui {

class State;

class TextEdit {
public:
  TextEdit(bool read_only, bool multiline, bool copyable);
  TextEdit(const TextEdit &) = delete;
  TextEdit &operator=(const TextEdit &) = delete;
  ~TextEdit();

  std::string_view text() const;

  void process(State &state);

  void set_text(std::string_view text);

private:
  nk_text_edit m_inner;
  nk_flags m_flags;
};

} // namespace shar::ui
