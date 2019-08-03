#include "options.hpp"
#include "ui/startup_screen.hpp"
#include "broadcast.hpp"
#include "view.hpp"


struct StartupHandler {
  using Startup = shar::ui::StartupScreen;

  void operator()(const Startup::Exit&) {
    exit = true;
  }

  void operator()(const Startup::Connect& connect) {
    options.connect = true;
    options.url = connect.url;
  }

  void operator()(const Startup::Stream& stream) {
    options.connect = false;
    options.url = stream.url;
    options.p2p = true;
  }

  shar::Options& options;

  bool exit{ false };
};

int main(int argc, const char* argv[]) {

  try {
    shar::Options options;

    if (argc > 1) {
      options = shar::Options::read(argc, argv);
    }
    else {
      shar::ui::StartupScreen screen;
      StartupHandler handler{ options };
      std::visit(handler, screen.run());

      if (handler.exit) {
        return EXIT_SUCCESS;
      }
    }

    if (options.connect) {
      shar::View view{ std::move(options) };
      return view.run();
    }
    else {
      shar::Broadcast broadcast{ std::move(options) };
      return broadcast.run();
    }
  }
  catch (const std::exception& e) {
    std::cerr << "An error occurred: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}