#pragma once

#include <memory>

#include "context.hpp"
#include "module.hpp"
#include "url.hpp"


namespace shar {

std::unique_ptr<INetworkModule> create_module(Context context, Url url);

}