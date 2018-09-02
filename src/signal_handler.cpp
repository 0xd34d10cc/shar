#include "signal_handler.hpp"

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <csignal>


// SIG_ERR expands to c-style cast
#include "disable_warnings_push.hpp"
static const sighandler_t SIGNAL_ERROR = SIG_ERR;
#include "disable_warnings_pop.hpp"

namespace {
static std::mutex              mutex;
static std::condition_variable signal_to_exit;
static std::atomic<bool>       is_running = false;


static void signal_handler(int /*signum*/) {
  std::lock_guard<std::mutex> lock(mutex);
  is_running = false;
  signal_to_exit.notify_all();
}
}

namespace shar {

// wait for ctrl+c
void SignalHandler::wait_for_sigint() {
  std::unique_lock<std::mutex> lock(mutex);
  while (is_running) {
    signal_to_exit.wait(lock);
  }
}

bool SignalHandler::setup() {
  is_running = true;
  return signal(SIGINT, signal_handler) != SIGNAL_ERROR;
}

}