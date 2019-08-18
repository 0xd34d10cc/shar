#include "header.hpp"

namespace shar::net::rtsp {

Header::Header(std::string_view key, std::string_view value)
  : key(std::move(key))
  , value(std::move(value)) {}

bool Header::operator==(const Header& rhs) const{

  return key == rhs.key && value == rhs.value;
}

bool Header::empty() const noexcept {

  return key.empty() && value.empty();
}

}