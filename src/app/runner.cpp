#include "runner.hpp"

#include "options.hpp"
#include "app.hpp"


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
    std::cerr << "An error occurred: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}

}