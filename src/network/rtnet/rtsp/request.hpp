#include <string>
#include <vector>
#include <utility>

#include "parser.hpp"

namespace shar::rtsp {

class Request {
  
public:

  enum class Type {
    OPTIONS,
    DESCRIBE,
    SETUP,
    TEARDOWN,
    PLAY,
    PAUSE,
    GET_PARAMETER,
    SET_PARAMETER,
    REDIRECT,
    ANNOUNCE,
    RECORD
  };

  Type                type() const noexcept;
  const std::string&  address() const noexcept;
  std::size_t         version() const noexcept;
  const std::vector<Header>& headers() const noexcept;
  const std::string&  body() const noexcept;

  void set_type(Type type);
  void set_address(std::string address);
  void set_version(std::size_t version);
  void add_header(std::string key, std::string value);
  void set_body(std::string body);

  static Request parse(const char* buffer, std::size_t size);

  bool serialize(char* destionation, std::size_t);

private:

  Type                m_type;
  std::string         m_address;
  std::size_t         m_version;
  std::vector<Header> m_headers;
  std::string         m_body;
};

}