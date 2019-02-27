#pragma once

#include <chrono>


namespace shar {

using Clock = std::chrono::high_resolution_clock;
using TimePoint = Clock::time_point;
using Milliseconds = std::chrono::milliseconds;

}