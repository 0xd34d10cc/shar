#include "context.hpp"
#include "net/sender.hpp"
#include "response.hpp"
#include "request.hpp"
#include "net/types.hpp"
#include "cancellation.hpp"


namespace shar::net::rtsp {

using codec::ffmpeg::Unit;

class ResponseSender :
  public IPacketSender,
  protected Context
{
public:
  ResponseSender(Context context, IpAddress ip, Port port);
  ResponseSender(const ResponseSender&) = delete;
  ResponseSender(ResponseSender&&) = delete;
  ResponseSender& operator=(const ResponseSender&) = delete;
  ResponseSender& operator=(ResponseSender&&) = delete;
  ~ResponseSender() override = default;

  void run(Receiver<Unit> packets) override;
  void shutdown() override;

private:
  void start_accepting();

  struct Client {
    explicit Client(tcp::Socket&& socket);


    tcp::Socket m_socket;
    std::vector<std::uint8_t> m_buffer;
    std::size_t               m_received_bytes;
    std::size_t               m_sent_bytes;
    std::vector<Header>       m_headers;
    Response                  m_response;
    std::size_t               m_response_size;
  };

  using ClientId = std::size_t;
  using Clients = std::unordered_map<ClientId, Client>;
  using ClientPos = Clients::iterator;
  void receive_request(ClientPos client);
  void send_response(ClientPos client);
  void disconnect_client(ClientPos client);
  void proccess_request(ClientPos client, Request request);
  Cancellation   m_running;


  IpAddress      m_ip;
  Port           m_port;
  IOContext      m_context;
  tcp::Socket    m_current_socket;
  tcp::Acceptor  m_acceptor;
  Clients        m_clients;
};
}