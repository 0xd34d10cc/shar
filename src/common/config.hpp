#pragma once

#include <string>
#include <vector>

#include "int.hpp"


namespace shar {

#ifdef SHAR_DEBUG_BUILD
#define DEFAULT_LOG_LEVEL LogLevel::Debug
#else
#define DEFAULT_LOG_LEVEL LogLevel::None
#endif


enum class LogLevel {
  Trace,
  Debug,
  Info,
  Warning,
  Error,
  Critical,
  None
};

struct Config {
  bool connect{ false };                             // true for receiver, false for sender
                                                     // if true all other options except
                                                     // for url are irrelevant

  bool p2p{ false };                                 // enable p2p mode, used only for sender
  std::string url{ "tcp://127.0.0.1:1337" };         // url to stream to
  usize monitor{ 0 };                          // which monitor to capture
  usize fps{ 30 };                             // desired fps (for encoder)
  std::string codec;                                 // which codec to use
  usize bitrate{ 5000 };                       // target bitrate (in kbits)
  std::string metrics{ "127.0.0.1:3228" };           // where to expose metrics
  LogLevel log_level{ DEFAULT_LOG_LEVEL };           // common log level
  std::string logs_location{ "" };                   // logs location, set to ~/.shar/logs
                                                     // by default
  LogLevel encoder_log_level{ DEFAULT_LOG_LEVEL };   // log level for encoder
  using Option = std::pair<std::string, std::string>;// name -> value
  std::vector<Option> options{                       // options for codec
    {"preset", "medium"},
    {"tune", "zerolatency"}
  };

  // reads config from command line arguments and config file (--config option, if provided)
  // WARNING: calls std::exit on error
  static Config from_args(int argc, char* argv[]);

  // convert config to json string
  std::string to_string() const;
};

}