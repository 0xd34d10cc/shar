#include "button.hpp"

#include "ui/state.hpp"


extern "C" int nk_button_label(nk_context*, const char*);

namespace shar::ui {

Button::Button(std::string label)
  : m_label(std::move(label))
{}

bool Button::process(State& state) {
  return nk_button_label(state.context(), m_label.c_str()) != 0;
}

}