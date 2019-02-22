#include "header.hpp"

namespace shar::rtsp {

Header::Header(std::string key, std::string value)
  : key(std::move(key))
  , value(std::move(value)) {}

bool Header::operator==(const Header& rhs) const{
  return key == rhs.key && value == rhs.value;
}
}