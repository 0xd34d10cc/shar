#pragma once

#include <string>


namespace shar::ui {

class State;

class Button {
public:
  Button(std::string label);
  Button(const Button&) = default;
  Button(Button&&) = default;
  Button& operator=(const Button&) = default;
  Button& operator=(Button&&) = default;
  ~Button() = default;

  // returns true if button was clicked
  bool process(State& state);

private:
  std::string m_label;
};

}