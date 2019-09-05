#include "runner.hpp"

#include "disable_warnings_push.hpp"
#include <SDL2/SDL_main.h>
#include "disable_warnings_pop.hpp"


int main(int argc, char* argv[]) {
  shar::Runner app_runner;
  return app_runner.run(argc, argv);
}