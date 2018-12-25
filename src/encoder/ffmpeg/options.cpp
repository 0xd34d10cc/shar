#include "options.hpp"

namespace shar::codecs::ffmpeg {
Options::~Options() {
  av_dict_free(&m_opts);
}

std::size_t Options::count() {
  return static_cast<std::size_t>(av_dict_count(m_opts));
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
    std::free(buffer);
    return {};
  }

  std::string opts{ buffer };
  return opts;
}
}