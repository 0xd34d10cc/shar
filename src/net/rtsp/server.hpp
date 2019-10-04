#include <vector>
#include <cstdlib> // std::size_t
#include <unordered_map>

#include "context.hpp"
#include "cancellation.hpp"
#include "net/sender.hpp"
#include "net/types.hpp"
#include "response.hpp"
#include "request.hpp"


namespace shar::net::rtsp {

using codec::ffmpeg::Unit;

class Server :
  public IPacketSender,
  protected Context
{
public:
  Server(Context context, IpAddress ip, Port port);
  Server(const Server&) = delete;
  Server(Server&&) = delete;
  Server& operator=(const Server&) = delete;
  Server& operator=(Server&&) = delete;
  ~Server() override = default;

  void run(Receiver<Unit> packets) override;
  void shutdown() override;

private:
  struct Client {
    explicit Client(tcp::Socket&& socket);

    tcp::Socket m_socket;
    std::vector<std::uint8_t> m_buffer;
    std::size_t m_received_bytes;
    std::size_t m_sent_bytes;

    std::size_t m_response_size;

    std::vector<Header> m_headers;
    // buffer to store headers values
    std::vector<char> m_headers_info_buffer;
  };

  using ClientId = std::size_t;
  using Clients = std::unordered_map<ClientId, Client>;
  using ClientPos = Clients::iterator;

  void start_accepting();
  void receive_request(ClientPos client);
  void send_response(ClientPos client);
  void disconnect(ClientPos client);

  Response proccess_request(ClientPos client, Request request);

  Cancellation   m_running;

  IpAddress      m_ip;
  Port           m_port;
  IOContext      m_context;
  tcp::Socket    m_current_socket;
  tcp::Acceptor  m_acceptor;
  Clients        m_clients;
};
}