#include "runner.hpp"

#include "disable_warnings_push.hpp"
#include <fmt/format.h>
#include "disable_warnings_pop.hpp"

#include "options.hpp"
#include "app.hpp"
#include "ui/message_box.hpp"


namespace shar {

Runner::Runner() {}
Runner::~Runner() {}

int Runner::run(int argc, char* argv[]) {
  try {
    auto options = Options::read(argc, argv);
    App app{ std::move(options) };
    return app.run();
  }
  catch (const std::exception& e) {
    ui::MessageBox error{
      ui::MessageBox::Type::Error,
      "Fatal error",
      fmt::format("Unhandled exception: {}", e.what())
    };
    error.show();
    return EXIT_FAILURE;
  }
}

}