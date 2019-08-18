#pragma once

#include <memory>

#include "context.hpp"
#include "url.hpp"
#include "receiver.hpp"


namespace shar::net {

std::unique_ptr<IPacketReceiver> create_receiver(Context context, Url url);

}