#include <iostream>
#include <string>

#include "config/serverConfig.h"
#include "net/asioLineServer.h"
#include "serverEngine.h"

int main(int argc, char** argv) {
  const simpleelo::server::ServerConfig config = simpleelo::server::loadServerConfig(argc, argv);

  simpleelo::server::ServerEngine engine(config.dataFilePath);
  if (!engine.load()) {
    std::cerr << "load database failed: " << config.dataFilePath << std::endl;
  }

  try {
    std::cout << "SimpleElo server dataFile=" << config.dataFilePath << std::endl;
    simpleelo::server::net::runAsioLineServer(
        config.listenHost,
        config.listenPort,
        [&engine](const std::string& request) { return engine.handleRequest(request); });
  } catch (const std::exception& ex) {
    std::cerr << "server startup failed: " << ex.what() << std::endl;
    return 1;
  }
}
