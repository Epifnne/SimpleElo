#include <iostream>
#include <istream>
#include <string>

#include <boost/asio.hpp>

#include "config/serverConfig.h"
#include "serverEngine.h"

int main(int argc, char** argv) {
  const simpleelo::server::ServerConfig config = simpleelo::server::loadServerConfig(argc, argv);

  simpleelo::server::ServerEngine engine(config.dataFilePath);
  if (!engine.load()) {
    std::cerr << "load database failed: " << config.dataFilePath << std::endl;
  }

  try {
    boost::asio::io_context ioContext;
    boost::asio::ip::tcp::endpoint endpoint(
        boost::asio::ip::make_address(config.listenHost), config.listenPort);
    boost::asio::ip::tcp::acceptor acceptor(ioContext);
    acceptor.open(endpoint.protocol());
    acceptor.set_option(boost::asio::socket_base::reuse_address(true));
    acceptor.bind(endpoint);
    acceptor.listen(boost::asio::socket_base::max_listen_connections);

    std::cout << "SimpleElo server listening on " << config.listenHost << ":" << config.listenPort
              << " dataFile=" << config.dataFilePath << std::endl;

    while (true) {
      boost::asio::ip::tcp::socket socket(ioContext);
      acceptor.accept(socket);

      try {
        boost::asio::streambuf requestBuffer;
        boost::asio::read_until(socket, requestBuffer, '\n');

        std::istream requestStream(&requestBuffer);
        std::string request;
        std::getline(requestStream, request);

        const std::string response = engine.handleRequest(request) + "\n";
        boost::asio::write(socket, boost::asio::buffer(response));
      } catch (const std::exception& ex) {
        std::cerr << "client session error: " << ex.what() << std::endl;
      }
    }
  } catch (const std::exception& ex) {
    std::cerr << "server startup failed: " << ex.what() << std::endl;
    return 1;
  }
}
