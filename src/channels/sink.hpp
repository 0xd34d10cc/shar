#pragma once

#include <optional>


namespace shar::channel {

// fake sender which is always connected and never full
template <typename T>
struct Sink {
  std::optional<T> send(T /* value */) {
    return std::nullopt;
  }

  bool connected() const {
    return true;
  }
};

}