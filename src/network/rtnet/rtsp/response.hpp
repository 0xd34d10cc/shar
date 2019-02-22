#include <string>
#include <vector>

#include "header.hpp"
namespace shar::rtsp {

class Response {

private:
  std::string         m_version;
  std::uint16_t       m_status_code;
  std::string         m_reason;

  std::vector<Header> m_headers;
};

}