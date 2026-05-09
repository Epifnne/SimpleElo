#include "config/serverConfig.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>

#include <nlohmann/json.hpp>

namespace simpleelo::server {
namespace {

std::string readEnvOrDefault(const char* name, const std::string& fallback) {
  if (const char* value = std::getenv(name)) {
    return value;
  }
  return fallback;
}

std::uint16_t parsePortOrDefault(const std::string& value, std::uint16_t fallback, const char* source) {
  try {
    const int parsed = std::stoi(value);
    if (parsed < 1 || parsed > static_cast<int>(std::numeric_limits<std::uint16_t>::max())) {
      std::cerr << "Ignore invalid port from " << source << ": " << value << std::endl;
      return fallback;
    }
    return static_cast<std::uint16_t>(parsed);
  } catch (...) {
    std::cerr << "Ignore invalid port from " << source << ": " << value << std::endl;
    return fallback;
  }
}

void applyConfigFile(ServerConfig& config) {
  if (!std::filesystem::exists(config.configFilePath)) {
    return;
  }

  std::ifstream fin(config.configFilePath);
  if (!fin.is_open()) {
    std::cerr << "Cannot open config file: " << config.configFilePath << std::endl;
    return;
  }

  nlohmann::json doc;
  try {
    fin >> doc;
  } catch (...) {
    std::cerr << "Cannot parse config file: " << config.configFilePath << std::endl;
    return;
  }

  if (doc.contains("listenHost") && doc["listenHost"].is_string()) {
    config.listenHost = doc["listenHost"].get<std::string>();
  }
  if (doc.contains("listenPort") && doc["listenPort"].is_number_integer()) {
    const int port = doc["listenPort"].get<int>();
    if (port >= 1 && port <= static_cast<int>(std::numeric_limits<std::uint16_t>::max())) {
      config.listenPort = static_cast<std::uint16_t>(port);
    }
  }
  if (doc.contains("dataFilePath") && doc["dataFilePath"].is_string()) {
    config.dataFilePath = doc["dataFilePath"].get<std::string>();
  }
}

void applyEnv(ServerConfig& config) {
  config.configFilePath = readEnvOrDefault("SIMPLEELO_SERVER_CONFIG", config.configFilePath);
  config.listenHost = readEnvOrDefault("SIMPLEELO_SERVER_HOST", config.listenHost);
  config.dataFilePath = readEnvOrDefault("SIMPLEELO_SERVER_DATA_FILE", config.dataFilePath);
  const std::string portValue = readEnvOrDefault("SIMPLEELO_SERVER_PORT", std::to_string(config.listenPort));
  config.listenPort = parsePortOrDefault(portValue, config.listenPort, "SIMPLEELO_SERVER_PORT");
}

void applyArgs(ServerConfig& config, int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--config" && i + 1 < argc) {
      config.configFilePath = argv[++i];
      continue;
    }
    if (arg == "--host" && i + 1 < argc) {
      config.listenHost = argv[++i];
      continue;
    }
    if (arg == "--port" && i + 1 < argc) {
      config.listenPort = parsePortOrDefault(argv[++i], config.listenPort, "--port");
      continue;
    }
    if (arg == "--data-file" && i + 1 < argc) {
      config.dataFilePath = argv[++i];
      continue;
    }
  }
}

}  // namespace

ServerConfig loadServerConfig(int argc, char** argv) {
  ServerConfig config;

  // Phase 1: command line can redirect config file path first.
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == "--config" && i + 1 < argc) {
      config.configFilePath = argv[i + 1];
      break;
    }
  }

  // Phase 2: env can redirect config file path if cli is absent.
  config.configFilePath = readEnvOrDefault("SIMPLEELO_SERVER_CONFIG", config.configFilePath);

  // Phase 3: apply config file baseline.
  applyConfigFile(config);

  // Phase 4: override with env.
  applyEnv(config);

  // Phase 5: override with cli.
  applyArgs(config, argc, argv);

  return config;
}

}  // namespace simpleelo::server
