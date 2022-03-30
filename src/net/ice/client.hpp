#pragma once

#include "net/types.hpp"

#include "disable_warnings_push.hpp"
#include <protocol.pb.h>
#include "disable_warnings_pop.hpp"

#include <functional>
#include <string>
#include <deque>


namespace shar::net::ice {

using SessionID = std::string;
using ConnectionInfo = std::string;
using RequestID = u32;

class Client {
public:
  Client(IOContext&);

  bool connected() const { return m_connected; }

  void connect(
    IpAddress ip,
    Port port,
    std::function<void(ErrorCode ec)> on_connect
  );
  void disconnect(ErrorCode ec);

  void open_session(
    std::string_view name,
    std::function<void(SessionID)> on_open,
    std::function<ConnectionInfo(ConnectionInfo)> on_connect
  );

  void connect_to_session(
    SessionID session_id,
    ConnectionInfo info,
    std::function<void(ConnectionInfo)>
  );

private:
  void send_message(proto::ClientMessage message);
  void start_send();

  IOContext& m_context;
  RequestID m_request_id{ 0 };
  bool m_connected{ false };
  tcp::Socket m_socket;

  std::function<void(ErrorCode ec)> m_on_connect;

  bool m_send_running{ false };
  std::deque<proto::ClientMessage> m_messages;
  std::vector<u8> m_send_buffer;
  usize m_sent{ 0 };
};

} // namespace shar::net