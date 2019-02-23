#include "app.hpp"


int main(int argc, const char* argv[]) {
  shar::App app{ argc, argv };

  try {
    return app.run();
  }
  catch (const std::exception& e) {
    std::cerr << "An error occurred: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}