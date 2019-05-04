#pragma once

#include <string>
#include <vector>
#include <cstdlib>


namespace shar {

enum class LogLevel {
  Quiet,
  Trace,
  Debug,
  Info,
  Warning,
  Error,
  Critical
};

struct Options {
  bool connect{ false };                             // true for receiver, false for sender
                                                     // if true all other options except
                                                     // for url are irrelevant

  std::string url;                                   // url to stream to
  std::size_t monitor{ 0 };                          // which monitor to capture
  std::size_t fps{ 30 };                             // desired fps (for encoder)
  std::string codec;                                 // which codec to use
  std::size_t bitrate{ 5000 };                       // target bitrate (in kbits)
  std::string metrics{ "127.0.0.1:3228" };           // where to expose metrics
  LogLevel loglvl{ LogLevel::Info };                 // loglvl for shar logger
  std::string log_file{ "shar.log" };                // name of logfile
  LogLevel encoder_loglvl{ LogLevel::Critical };     // loglvl for encoder logger
  using Option = std::pair<std::string, std::string>;
  std::vector<Option> options;                       // options for codec

  // reads options from command line arguments, config and environment variables
  static Options read(int argc, const char* argv[]);
};

}