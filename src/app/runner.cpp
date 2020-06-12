#include "runner.hpp"

#include "disable_warnings_push.hpp"
#include <fmt/format.h>
#include "disable_warnings_pop.hpp"

#include "config.hpp"
#include "app.hpp"
#include "ui/controls/message_box.hpp"


namespace shar {

Runner::Runner() {}
Runner::~Runner() {}

int Runner::run(int argc, char* argv[]) {
  try {
    auto config = Config::from_args(argc, argv);
    App app{ std::move(config) };
    return app.run();
  }
  catch (const std::exception& e) {
    auto error = ui::MessageBox::error(
      "Fatal error",
      fmt::format("Unhandled exception: {}", e.what())
    );
    error.show();
    return EXIT_FAILURE;
  }
}

}