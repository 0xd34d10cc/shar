#include <stdexcept>

#include "disable_warnings_push.hpp"
#include <miniupnpc.h>
#include <upnpcommands.h>
#include "disable_warnings_pop.hpp"

#include "forwarding.hpp"


namespace shar::net::tcp {

// TODO: extract port forwarding to a class
void forward_port(Port local, Port remote, Logger& logger) {
  int           error      = 0;
  //get a list of upnp devices (asks on the broadcast address and returns the responses)
  struct UPNPDev* upnp_dev = upnpDiscover(1000,     //timeout in milliseconds
                                          nullptr,  //multicast address, default = "239.255.255.250"
                                          nullptr,  //minissdpd socket, default = "/var/run/minissdpd.sock"
                                          0,        //source port, default = 1900
                                          0,        //0 = IPv4, 1 = IPv6
                                          3,        // ttl
                                          &error);  //error output

  if (upnp_dev == nullptr || error != 0) {
    freeUPNPDevlist(upnp_dev);
    throw std::runtime_error("Could not discover upnp device");
  }

  static const int IPV6_MAX_LEN = 46; //maximum length of an ipv6 address string

  char            lan_address[IPV6_MAX_LEN];
  struct UPNPUrls upnp_urls;
  struct IGDdatas upnp_data;
  error = UPNP_GetValidIGD(upnp_dev, &upnp_urls, &upnp_data, lan_address, sizeof(lan_address));

  //there are more status codes in minupnpc.c but 1 is success all others are different failures
  if (error != 1) {
    FreeUPNPUrls(&upnp_urls);
    freeUPNPDevlist(upnp_dev);
    throw std::runtime_error("No valid Internet Gateway Device could be connected to");
  }

  // get the external (WAN) IP address
  char wan_address[IPV6_MAX_LEN];
  if (UPNP_GetExternalIPAddress(upnp_urls.controlURL, upnp_data.first.servicetype, wan_address) != 0) {
    FreeUPNPUrls(&upnp_urls);
    freeUPNPDevlist(upnp_dev);
    throw std::runtime_error("Could not get external IP address");
  }

  logger.info("External IP: {}", wan_address);

  std::string remote_port = std::to_string(remote);
  std::string local_port = std::to_string(local);

  // add a new TCP port mapping from WAN port |remote| to local host port 24680
  error = UPNP_AddPortMapping(
      upnp_urls.controlURL,
      upnp_data.first.servicetype,
      remote_port.c_str(),  // external (WAN) port requested
      local_port.c_str(),  // internal (LAN) port to which packets will be redirected
      lan_address, // internal (LAN) address to which packets will be redirected
      "Share server", // text description to indicate why or who is responsible for the port mapping
      "TCP", // protocol must be either TCP or UDP
      nullptr, // remote (peer) host address or nullptr for no restriction
      "86400"); // port map lease duration (in seconds) or zero for "as long as possible"

  if (error) {
    FreeUPNPUrls(&upnp_urls);
    freeUPNPDevlist(upnp_dev);
    throw std::runtime_error("Failed to map ports");
  }

  logger.info("Successfully mapped port {} (local) to {} (remote)", local, remote);

  FreeUPNPUrls(&upnp_urls);
  freeUPNPDevlist(upnp_dev);
}

}