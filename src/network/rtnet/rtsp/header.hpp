#include <string>


namespace shar::rtsp {
struct Header {

  Header() = delete;

  Header(std::string key, std::string value);

  Header(const Header&) = default;
  Header(Header&&) = default;

  Header& operator=(const Header&) = default;
  Header& operator=(Header&&) = default;

  bool operator==(const Header& rhs) const;

  std::string key;
  std::string value;
};

}