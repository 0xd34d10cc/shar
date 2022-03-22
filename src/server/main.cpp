#include <cstdlib>

#include "logger.hpp"
#include "server.hpp"

int main() {
  try {
    shar::init_log(".", shar::LogLevel::Info);
    shar::Server server;
    server.run();
    return EXIT_SUCCESS;
  } catch (const std::exception& e) {
    LOG_FATAL("Error: {}", e.what());
    return EXIT_FAILURE;
  }
}