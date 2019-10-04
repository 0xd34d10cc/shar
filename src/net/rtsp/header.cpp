#include <algorithm>

#include "header.hpp"


namespace shar::net::rtsp {

Header::Header(Bytes name, Bytes value)
  : name(std::move(name))
  , value(std::move(value))
{}

bool Header::operator==(const Header& rhs) const{
  return name == rhs.name && value == rhs.value;
}

bool Header::empty() const noexcept {
  return name.empty() && value.empty();
}

Headers::Headers(Header* ptr, std::size_t size)
  : data(ptr)
  , len(size)
  {}

Header* Headers::begin() {
  return data;
}

Header* Headers::end() {
  return data + len;
}

std::optional<Header> Headers::get(Bytes name) {
  auto it = std::find_if(begin(), end(), [name](const auto& h) {
    return h.name == name;
  });

  if (it == end()) {
    return std::nullopt;
  }

  return *it;
}

}