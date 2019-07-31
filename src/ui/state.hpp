#pragma once

#include <memory>


struct nk_context;
union SDL_Event;

namespace shar::ui {

// GUI state
class State {
public:
  State();
  State(State&&) = default;
  State& operator=(State&&) = default;
  ~State();

  const nk_context* context() const;
  nk_context* context();

  // returns true if event was processed
  bool handle(SDL_Event* event);
  void clear();

private:
  std::unique_ptr<nk_context> m_context;
};

}