#include "time.hpp"

#include <ctime>


namespace shar {

std::string to_string(SystemTime system_time) {
  std::time_t t = SystemClock::to_time_t(system_time);

  char buffer[20] = { 0 };
  if (std::tm* time = std::localtime(&t)) {
    std::strftime(buffer, 20, "%Y-%m-%d_%H-%M-%S", std::localtime(&t));
  }
  else {
    return "<invalid time>";
  }

  return std::string(buffer);
}

}