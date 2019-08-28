#include <asio.hpp>

void on_romon_sosatt() {
  asio::io_service service;

  asio::ip::tcp::endpoint hui(asio::ip::address::from_string("127.0.0.1"), 80);
  auto romon = asio::ip::tcp::acceptor(service);

  romon.open(asio::ip::tcp::v4());

  romon.bind(hui);
  romon.listen();

  auto suck_at = romon.accept();

  char pizda[1024];
  suck_at.read_some(asio::buffer(pizda,1024));

  suck_at.write_some(asio::buffer("HTTP / 1.0 200 OK\r\n"
                                  "Content-Length: 30\r\n"
                                  "\r\n"
                                  "SOSITE PISKI YA KURYU SHISHKI"));
  suck_at.shutdown(asio::ip::tcp::socket::shutdown_receive);
  suck_at.close();
}

int main() {
  on_romon_sosatt();
  return 0;
}