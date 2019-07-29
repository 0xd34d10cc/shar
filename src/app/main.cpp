#include "options.hpp"
#include "broadcast.hpp"
#include "view.hpp"

#include "ui/window.hpp"


int main(int argc, const char* argv[]) {
  try {
    auto options = shar::Options::read(argc, argv);

    if (options.connect) {
      shar::ui::run();

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