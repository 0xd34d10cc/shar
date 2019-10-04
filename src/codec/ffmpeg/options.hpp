#pragma once

#include <string>

#include "int.hpp"


struct AVDictionary;

namespace shar::codec::ffmpeg {

class Options {
public:
  Options() = default;
  Options(const Options&) = delete;
  Options& operator=(const Options&) = delete;
  ~Options();

  usize count();
  bool set(const char* key, const char* value);
  AVDictionary*& get_ptr();
  std::string to_string();

private:
  AVDictionary* m_opts = nullptr;
};

}