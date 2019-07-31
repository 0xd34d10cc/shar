#include "options.hpp"
#include "broadcast.hpp"
#include "view.hpp"


int main(int argc, const char* argv[]) {
  try {
    auto options = shar::Options::read(argc, argv);
    shar::View view{ std::move(options) };
    return view.run();
  }
  catch (const std::exception& e) {
    std::cerr << "An error occurred: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}