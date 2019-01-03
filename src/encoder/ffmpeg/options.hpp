#pragma once
#include <cassert>
#include <cstdlib>
#include <vector>
#include <numeric>

#include "disable_warnings_push.hpp"
extern "C" {
#include <libavcodec/avcodec.h>
}
#include "disable_warnings_pop.hpp"

#include "size.hpp"
#include "logger.hpp"
#include "config.hpp"
#include "network/packet.hpp"
#include "capture/frame.hpp"

namespace shar::codecs::ffmpeg {
class Options {
public:
  Options() = default;
  Options(const Options&) = delete;
  Options& operator=(const Options&) = delete;
  ~Options();

  std::size_t count();
  bool set(const char* key, const char* value);
  AVDictionary*& get_ptr();
  std::string to_string();

private:
  AVDictionary* m_opts = nullptr;
};
}