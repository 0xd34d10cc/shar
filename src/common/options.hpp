#pragma once

#include <string>
#include <cstdlib>


namespace shar {

struct Options {
  std::string url;                           // url to stream to
  std::size_t monitor{ 0 };                  // which monitor to capture
  std::size_t fps{ 30 };                     // desired fps (for encoder)
  std::string codec;                         // which codec to use
  std::size_t bitrate{ 5000 };               // target bitrate (in kbits)
  std::string metrics{ "127.0.0.1:3228" };   // where to expose metrics
  std::string loglvl{ "info" };             // loglvl for shar logger
  std::string log_file{ "shar.log" };        // name of logfile 
  std::string encoder_loglvl{ "panic" };     // loglvl for encoder logger 
  using Option = std::pair<std::string, std::string>;
  std::vector<Option> options;               // options for codec

  // reads options from command line arguments, config and environment variables
  static Options read(int argc, const char* argv[]);
};

}