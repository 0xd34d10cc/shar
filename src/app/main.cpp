#include "config.hpp"
#include "app.hpp"
#include "ui/controls/message_box.hpp"

#include "disable_warnings_push.hpp"
#include <fmt/format.h>
#include <SDL2/SDL_main.h>
#include "disable_warnings_pop.hpp"


int main(int argc, char* argv[]) {
  try {
    auto config = shar::Config::from_args(argc, argv);
    shar::App app{std::move(config)};
    return app.run();
  } catch (const std::exception& e) {
    const auto message = fmt::format("Unhandled exception: {}", e.what());
    auto error =
        shar::ui::MessageBox::error("Fatal error", message);
    error.show();
    return EXIT_FAILURE;
  }
}