#include "disable_warnings_push.hpp"
#include <CLI/CLI.hpp>
#include <nlohmann/json.hpp>
#include <fmt/format.h>
#include "disable_warnings_pop.hpp"

#include "options.hpp"


namespace {
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
}


namespace shar {

Options Options::read(int argc, const char* argv[]) {
  Options opts{}; // make default
  std::vector<std::string> codec_options;

  CLI::App app{"shar - yet another tool for video streaming"};
  app.config_formatter(std::make_shared<ConfigJSON>());

  app.set_config("-c,--config");
  app.add_option("url,-u,--url", opts.url, "Url to stream to")->required(true);
  app.add_option("-m,--monitor", opts.monitor, "Which monitor to capture");
  app.add_option("-f,--fps", opts.fps, "Desired fps", true);
  app.add_option("--codec", opts.codec, "Which codec to use");
  app.add_option("-b,--bitrate", opts.bitrate, "Target bitrate (kbit)", true);
  app.add_option("--metrics", opts.metrics, "Where to expose metrics", true);
  app.add_option("-o,--options", codec_options, "Codec options, in key=value format");

  try {
    app.parse(argc, argv);
  }
  catch (const CLI::ParseError& e) {
    int code = app.exit(e);
    std::exit(code);
  }

  if (!codec_options.empty()) {
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

  fmt::print("Running with:\n{}\n", app.config_to_str(true, true));
  return opts;
}

}