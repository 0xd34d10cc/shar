#include "runner.hpp"

#include "disable_warnings_push.hpp"
#include <fmt/format.h>
#include "disable_warnings_pop.hpp"

#include "config.hpp"
#include "app.hpp"
#include "metric_collector.hpp"
#include "ui/message_box.hpp"


namespace shar {

Runner::Runner() {}
Runner::~Runner() {}

int Runner::run(int argc, char* argv[]) {
  try {
    auto config = Config::from_args(argc, argv);
    MetricCollector metric_collector;
    App app{ std::move(config), std::make_shared<MetricCollector>(metric_collector) };
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