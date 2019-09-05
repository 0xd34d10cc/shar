#pragma once

#include <string>

#include "nk.hpp"


namespace shar::ui {

class State;

class TextEdit {
public:
  TextEdit(bool read_only, bool multiline);
  TextEdit(const TextEdit&) = delete;
  TextEdit& operator=(const TextEdit&) = delete;
  ~TextEdit();

  std::string text() const;

  void process(State& state);

  void set_text(const std::string& text);

private:
  nk_text_edit m_inner;
  nk_flags m_flags;
};

}