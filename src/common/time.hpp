#pragma once

#include <chrono>
#include <string>


namespace shar {

using Clock = std::chrono::high_resolution_clock;
using TimePoint = Clock::time_point;

using SystemClock = std::chrono::system_clock;
using SystemTime = SystemClock::time_point;

using Milliseconds = std::chrono::milliseconds;

std::string to_string(SystemTime time);

}