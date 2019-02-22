#include <string>


namespace shar::rtsp {
struct Header {

  Header() = delete;

  Header(std::string key, std::string value)
    : m_key(std::move(key))
    , m_value(std::move(value)){}

  Header(const Header&) = default;
  Header(Header&&) = default;

  Header operator=(const Header&) = default;
  Header operator=(Header&&) = default;

  std::string key;
  std::string value;
};

}