#pragma once

#include "queues/queue.hpp"
#include "primitives/image.hpp"


namespace shar {
using FramesQueue = FixedSizeQueue<Image, 30>;
}