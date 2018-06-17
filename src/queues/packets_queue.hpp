#pragma once

#include "queues/queue.hpp"
#include "packet.hpp"


namespace shar {
using PacketsQueue = FixedSizeQueue<Packet, 120>;
}