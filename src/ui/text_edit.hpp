#pragma once

#include <string>

#include "nk.hpp"


namespace shar::ui {

class State;

class TextEdit {
public:
  TextEdit(bool read_only, bool multiline);
  ~TextEdit();

  std::string text() const;

  void process(State& state);

  void set_text(const std::string& text);

private:
  nk_text_edit m_inner;
  int m_flags;
};

}