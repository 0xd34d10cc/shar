#include "app.hpp"

#include "disable_warnings_push.hpp"
#include <SDL2/SDL_main.h>
#include "disable_warnings_pop.hpp"


int main(int argc, char* argv[]) {
  shar::App app;
  return app.run(argc, argv);
}