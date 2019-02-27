#include <string>
#include <vector>

#include "parser.hpp"


namespace shar::rtsp {

class Response {

public:
  std::size_t         version() const noexcept;
  std::uint16_t       status_code() const noexcept;
  const std::string&  reason() const noexcept;
  const std::vector<Header>& headers() const noexcept;
  const std::string&  body() const noexcept;

  void set_version(std::size_t version);
  void set_status_code(std::uint16_t statuc_code);
  void set_reason(std::string reason);
  void add_header(std::string key, std::string value);
  void set_body(std::string body);

  static Response parse(const char* buffer, std::size_t size);
  //return false if we haven't enough space in buffer
  bool serialize(char* destination, std::size_t size);

private:
  std::size_t         m_version;
  std::uint16_t       m_status_code;
  std::string         m_reason;

  std::vector<Header> m_headers;
  std::string         m_body;
};

}