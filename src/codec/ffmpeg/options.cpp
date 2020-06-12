#include "options.hpp"

#include "disable_warnings_push.hpp"
extern "C" {
#include <libavutil/dict.h>
#include <libavutil/mem.h>
}
#include "disable_warnings_pop.hpp"


namespace shar::codec::ffmpeg {

Options::~Options() {
  av_dict_free(&m_opts);
}

usize Options::count() {
  return static_cast<usize>(av_dict_count(m_opts));
}

bool Options::set(const char* key, const char* value) {
  return av_dict_set(&m_opts, key, value, 0 /* flags */) >= 0;
}

AVDictionary*& Options::get_ptr() {
  return m_opts;
}

std::string Options::to_string() {
  char* buffer = nullptr;
  if (av_dict_get_string(m_opts, &buffer, '=', ',') < 0) {
    av_free(buffer);
    return {};
  }

  std::string opts{ buffer };
  av_free(buffer);
  return opts;
}

}