#pragma once

#include <memory>

#include "context.hpp"
#include "sender.hpp"
#include "url.hpp"


namespace shar {

std::unique_ptr<IPacketSender> create_sender(Context context, Url url);

}