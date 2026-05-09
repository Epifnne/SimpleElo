#pragma once

#include <cstdint>
#include <string>

namespace simpleelo::server {

struct ServerConfig {
  std::string listenHost = "127.0.0.1";
  std::uint16_t listenPort = 18080;
  std::string dataFilePath = "serverData.json";
  std::string configFilePath = "server_config.json";
};

// Load server config with precedence:
// command line > environment variables > config file > defaults.
ServerConfig loadServerConfig(int argc, char** argv);

}  // namespace simpleelo::server
