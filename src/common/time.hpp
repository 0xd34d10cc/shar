#pragma once

#include <chrono>
#include <string>


namespace shar {

using Clock = std::conditional<std::chrono::high_resolution_clock::is_steady,
                               std::chrono::high_resolution_clock,
                               std::chrono::steady_clock>::type;
using TimePoint = Clock::time_point;

using SystemClock = std::chrono::system_clock;
using SystemTime = SystemClock::time_point;

using Microseconds = std::chrono::microseconds;
using Milliseconds = std::chrono::milliseconds;
using Seconds = std::chrono::seconds;
using Minutes = std::chrono::minutes;
using Hours = std::chrono::hours;

std::string to_string(SystemTime time);

}