#include "config.hpp"
#include "app.hpp"
#include "ui/controls/message_box.hpp"

#include "net/ice/candidate.hpp"
#include "common/error_or.hpp"

#include "disable_warnings_push.hpp"
#include <fmt/format.h>
#include <SDL2/SDL_main.h>
#include "disable_warnings_pop.hpp"

using shar::Config;
using shar::App;
using shar::ui::MessageBox;

using namespace shar;
using namespace shar::net;

int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;
  try {
    Logger logger(".", LogLevel::Trace);
    IOContext context;
    udp::Socket socket(context);
    auto ip = IpAddress(IPv4({0, 0, 0, 0}));
    auto endpoint = udp::Endpoint(ip, Port{44444});
    socket.open(udp::v4());
    socket.bind(endpoint);

    ErrorCode ec;
    auto candidates = ice::gather_candidates(socket, logger, ec);
    if (ec) {
      fmt::print("Failed: {}\n", ec.message());
      return EXIT_FAILURE;
    }

    std::vector<std::string> c;
    for (const auto& candidate : candidates) {
      c.push_back(fmt::format("{} {} {}",
                              static_cast<int>(candidate.type),
                              candidate.ip.to_string(),
                              candidate.port));
    }

    return EXIT_SUCCESS;
    //auto config = Config::from_args(argc, argv);
    //App app{std::move(config)};
    //return app.run();
  } catch (const std::exception& e) {
    const auto message = fmt::format("Unhandled exception: {}", e.what());
    auto error = MessageBox::error("Fatal error", message);
    error.show();
    return EXIT_FAILURE;
  }
}