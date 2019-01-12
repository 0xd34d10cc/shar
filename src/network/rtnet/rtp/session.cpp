#include "session.hpp"


namespace shar::rtp {
  
Session::Session(UDPSocket socket, Endpoint endpoint) noexcept
  : m_socket(std::move(socket))
  , m_endpoint(std::move(endpoint))
{}

}