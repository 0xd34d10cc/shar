#include <cstdlib> // std::getenv
#include <filesystem>

#include "disable_warnings_push.hpp"
#include <CLI/CLI.hpp>
#include <nlohmann/json.hpp>
#include <fmt/format.h>
#include "disable_warnings_pop.hpp"

#include "options.hpp"
#include "logger.hpp"


namespace {

static constexpr char config_name[]  = "config.json";
using nlohmann::json;

class ConfigJSON : public CLI::Config {
public:
  std::string to_config(const CLI::App *app, bool default_also, bool, std::string) const override {
    json j;

    for (const CLI::Option *opt : app->get_options({})) {

      // Only process option with a long-name and configurable
      if (!opt->get_lnames().empty() && opt->get_configurable()) {
        std::string name = opt->get_lnames()[0];

        // Non-flags
        if (opt->get_type_size() != 0) {

          // If the option was found on command line
          if (opt->count() == 1)
            j[name] = opt->results().at(0);
          else if (opt->count() > 1)
            j[name] = opt->results();

          // If the option has a default and is requested by optional argument
          else if (default_also && !opt->get_defaultval().empty())
            j[name] = opt->get_defaultval();

          // Flag, one passed
        }
        else if (opt->count() == 1) {
          j[name] = true;

          // Flag, multiple passed
        }
        else if (opt->count() > 1) {
          j[name] = opt->count();

          // Flag, not present
        }
        else if (opt->count() == 0 && default_also) {
          j[name] = false;
        }
      }
    }

    for (const CLI::App *subcom : app->get_subcommands({}))
      j[subcom->get_name()] = json(to_config(subcom, default_also, false, ""));

    return j.dump(4);
  }

  std::vector<CLI::ConfigItem> from_config(std::istream &input) const override {
    json j;
    input >> j;
    return _from_config(j);
  }

  std::vector<CLI::ConfigItem>
    _from_config(json j, std::string name = "", std::vector<std::string> prefix = {}) const {
    std::vector<CLI::ConfigItem> results;

    if (j.is_object()) {
      for (json::iterator item = j.begin(); item != j.end(); ++item) {
        auto copy_prefix = prefix;
        if (!name.empty())
          copy_prefix.push_back(name);
        auto sub_results = _from_config(*item, item.key(), copy_prefix);
        results.insert(results.end(), sub_results.begin(), sub_results.end());
      }
    }
    else if (!name.empty()) {
      results.emplace_back();
      CLI::ConfigItem &res = results.back();
      res.name = name;
      res.parents = prefix;
      if (j.is_boolean()) {
        res.inputs = { j.get<bool>() ? "true" : "false" };
      }
      else if (j.is_number()) {
        std::stringstream ss;
        ss << j.get<double>();
        res.inputs = { ss.str() };
      }
      else if (j.is_string()) {
        res.inputs = { j.get<std::string>() };
      }
      else if (j.is_array()) {
        for (std::string ival : j)
          res.inputs.push_back(ival);
      }
      else {
        throw CLI::ConversionError("Failed to convert " + name);
      }
    }
    else {
      throw CLI::ConversionError("You must make all top level values objects in json!");
    }

    return results;
  }
};

std::string log_level_to_string(shar::LogLevel loglevel) {

  switch(loglevel) {
    case(shar::LogLevel::None): return "none";
    case(shar::LogLevel::Trace): return "trace";
    case(shar::LogLevel::Debug): return "debug";
    case(shar::LogLevel::Info): return "info";
    case(shar::LogLevel::Warning): return "warning";
    case(shar::LogLevel::Error): return "error";
    case(shar::LogLevel::Critical): return "critical";
    default: assert(!"invalid loglevel, update log_level_to_string"); 
             throw std::runtime_error("invalid loglevel, update log_level_to_string");
  }}


std::string opts_to_string(const shar::Options& opts) {
  json j;
  std::vector<std::string> string_options;
  for (const auto& option : opts.options) { 
    string_options.push_back(fmt::format("{}={}", option.first, option.second));
  }

  j["bitrate"] = opts.bitrate;
  j["codec"] = opts.codec;
  j["connect"] = opts.connect;
  j["encoder_loglevel"] = log_level_to_string(opts.encoder_log_level);
  j["fps"] = opts.fps;
  j["logs"] = opts.logs_location;
  j["log_level"] = log_level_to_string(opts.log_level);
  j["metrics"] = opts.metrics;
  j["monitor"] = opts.monitor;
  j["options"] = string_options;
  j["p2p"] = opts.p2p;
  j["url"] = opts.url;

  return j.dump(4);
}

std::filesystem::path get_shar_path() {
  if (const char* home = std::getenv("HOME")) {
    return std::filesystem::path(home) / ".shar";
  }
  else if (const char* userprofile = std::getenv("userprofile")) {
    return std::filesystem::path(userprofile) / ".shar";
  }
  else {
    return std::filesystem::current_path();
  }
}

}


namespace shar {

static shar::LogLevel log_level_from_string(const std::string& str) {
  static std::map<std::string, shar::LogLevel> values = {
    {"none",     shar::LogLevel::None},
    {"trace",    shar::LogLevel::Trace},
    {"debug",    shar::LogLevel::Debug},
    {"info",     shar::LogLevel::Info},
    {"warning",  shar::LogLevel::Warning},
    {"error",    shar::LogLevel::Error},
    {"critical", shar::LogLevel::Critical}
  };

  auto it = values.find(str);
  if (it != values.end()) {
    return it->second;
  }

  throw std::runtime_error(fmt::format("Unknown log level: {}", str));
}

Options Options::read(int argc, char* argv[]) {
  Options opts;
  std::vector<std::string> codec_options;
  std::string loglvl;
  std::string encoder_loglvl;

  auto shar_dir = get_shar_path();

  std::set<std::string> loglvl_options{
    "none",
    "trace",
    "debug",
    "info",
    "warning",
    "error",
    "critical"
  };

  CLI::App app{"shar - yet another tool for video streaming"};
  app.config_formatter(std::make_shared<ConfigJSON>());

  if (std::filesystem::exists(shar_dir / config_name) && 
      std::filesystem::is_regular_file(shar_dir / config_name)) {
    app.set_config("-c,--config", (shar_dir / config_name).string());
  }
  else {
    app.set_config("-c,--config");
  }
  app.add_flag("--connect", opts.connect, "Connect to session");
  app.add_flag("--p2p", opts.p2p, "Enable p2p mode, makes sense only for sender");
  app.add_option("url,-u,--url", opts.url, "Url for stream or connect");
  app.add_option("-m,--monitor", opts.monitor, "Which monitor to capture");
  app.add_option("-f,--fps", opts.fps, "Desired fps", true);
  app.add_option("--codec", opts.codec, "Which codec to use");
  app.add_option("-b,--bitrate", opts.bitrate, "Target bitrate (kbit)", true);
  app.add_option("--metrics", opts.metrics, "Where to expose metrics", true);
  app.add_option("--logs", opts.logs_location, "Log files location", true);
  app.add_set("--log_level", loglvl, loglvl_options, "common log level", true);
  app.add_set("--encoder_loglevel", encoder_loglvl, loglvl_options, "log level for encoder", true);
  app.add_option("-o,--options", codec_options, "Codec options, in key=value format");

  try {
    app.parse(argc, argv);
  }
  catch (const CLI::ParseError& e) {
    int code = app.exit(e);
    std::exit(code);
  }

  if (!codec_options.empty()) {
    // clear default values
    opts.options.clear();

    for (const auto& option : codec_options) {
      auto it = std::find(option.begin(), option.end(), '=');
      if (it == option.end()) {
        fmt::print("Invalid codec option format: {}", option);
        std::exit(EXIT_FAILURE);
      }

      std::string key{ option.begin(), it };
      std::string value{ it+1, option.end() };
      opts.options.emplace_back(std::move(key), std::move(value));
    }

  }

  if (!loglvl.empty()) {
    opts.log_level = log_level_from_string(loglvl);
  }

  if (!encoder_loglvl.empty()) {
    opts.encoder_log_level = log_level_from_string(encoder_loglvl);
  }

  if (opts.logs_location.empty()) {
    opts.logs_location = (shar_dir / "logs").string();
  }

  return opts;
}

void Options::dump_options() {
  std::string json = opts_to_string(*this);
  std::ofstream config_stream(get_shar_path() / config_name);
  config_stream << json;
}

}