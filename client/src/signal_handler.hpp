#pragma once

namespace shar {

class SignalHandler {
public:
  static bool setup();
  static void wait_for_sigint();
};

}