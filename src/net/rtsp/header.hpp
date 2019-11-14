#pragma once

#include <optional>

#include "bytes_ref.hpp"


namespace shar::net::rtsp {

struct Header {
  Header() = default;
  Header(BytesRef name, BytesRef value);
  Header(const Header&) = default;
  Header(Header&&) = default;

  Header& operator=(const Header&) = default;
  Header& operator=(Header&&) = default;

  bool operator==(const Header& rhs) const;

  bool empty() const noexcept;

  BytesRef name;
  BytesRef value;
};

struct Headers {
  Headers(Header* ptr, usize size);
  Headers(const Headers&) = default;

  Header* begin();
  Header* end();

  std::optional<Header> get(BytesRef name);

  Header* data{ nullptr };
  usize len{ 0 };
};

}
