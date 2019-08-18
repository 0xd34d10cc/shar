#pragma once

#include <string_view>
#include <vector>


namespace shar::net::rtsp {

struct Header {
  Header() = default;

  Header(std::string_view key, std::string_view value);

  Header(const Header&) = default;
  Header(Header&&) = default;

  Header& operator=(const Header&) = default;
  Header& operator=(Header&&) = default;

  bool operator==(const Header& rhs) const;

  bool empty() const noexcept;

  std::string_view key;
  std::string_view value;
};

struct Headers {

  Header* data{ nullptr };
  std::size_t len{ 0 };
};

}