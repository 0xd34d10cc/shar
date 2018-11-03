#pragma once

#include <optional>


namespace shar::channel {

// fake receiver which always returns default-initialized value
template <typename T>
struct Source {
  std::optional<T> receive() {
    return T{};
  }

  bool connected() const {
    return true;
  }
};

}