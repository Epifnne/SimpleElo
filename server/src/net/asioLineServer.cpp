#include "net/asioLineServer.h"

#include <iostream>
#include <istream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include <boost/asio.hpp>

namespace simpleelo::server::net {
namespace {

boost::asio::ip::tcp::endpoint resolveEndpoint(boost::asio::io_context& ioContext,
                                               const std::string& host,
                                               std::uint16_t port) {
  boost::system::error_code ec;
  const auto address = boost::asio::ip::make_address(host, ec);
  if (!ec) {
    return boost::asio::ip::tcp::endpoint(address, port);
  }

  boost::asio::ip::tcp::resolver resolver(ioContext);
  const auto results = resolver.resolve(host, std::to_string(port), ec);
  if (ec || results.empty()) {
    throw std::runtime_error("cannot resolve host: " + host);
  }

  return results.begin()->endpoint();
}

class LineSession : public std::enable_shared_from_this<LineSession> {
 public:
  LineSession(boost::asio::ip::tcp::socket socket, const RequestHandler& handler)
      : socket_(std::move(socket)), handler_(handler) {}

  void start() {
    doReadLine();
  }

 private:
  void doReadLine() {
    auto self = shared_from_this();
    boost::asio::async_read_until(
        socket_,
        requestBuffer_,
        '\n',
        [self](const boost::system::error_code& ec, std::size_t) {
          if (ec) {
            self->close();
            return;
          }
          self->onReadCompleted();
        });
  }

  void onReadCompleted() {
    std::istream requestStream(&requestBuffer_);
    std::string request;
    std::getline(requestStream, request);
    if (!request.empty() && request.back() == '\r') {
      request.pop_back();
    }

    try {
      response_ = handler_(request) + "\n";
    } catch (const std::exception& ex) {
      std::cerr << "client session error: " << ex.what() << std::endl;
      close();
      return;
    }

    doWriteLine();
  }

  void doWriteLine() {
    auto self = shared_from_this();
    boost::asio::async_write(
        socket_,
        boost::asio::buffer(response_),
        [self](const boost::system::error_code& ec, std::size_t) {
          if (ec) {
            self->close();
            return;
          }
          self->close();
        });
  }

  void close() {
    boost::system::error_code ignored;
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored);
    socket_.close(ignored);
  }

  boost::asio::ip::tcp::socket socket_;
  boost::asio::streambuf requestBuffer_;
  std::string response_;
  RequestHandler handler_;
};

class AsioLineServer {
 public:
  AsioLineServer(boost::asio::io_context& ioContext,
                 const boost::asio::ip::tcp::endpoint& endpoint,
                 const RequestHandler& handler)
      : ioContext_(ioContext),
        acceptor_(ioContext),
        handler_(handler) {
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(boost::asio::socket_base::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen(boost::asio::socket_base::max_listen_connections);
  }

  void startAccept() {
    doAccept();
  }

 private:
  void doAccept() {
    acceptor_.async_accept(
        ioContext_,
        [this](const boost::system::error_code& ec, boost::asio::ip::tcp::socket socket) {
          if (!ec) {
            std::make_shared<LineSession>(std::move(socket), handler_)->start();
          } else {
            std::cerr << "accept error: " << ec.message() << std::endl;
          }
          doAccept();
        });
  }

  boost::asio::io_context& ioContext_;
  boost::asio::ip::tcp::acceptor acceptor_;
  RequestHandler handler_;
};

}  // namespace

void runAsioLineServer(const std::string& host, std::uint16_t port, const RequestHandler& handler) {
  boost::asio::io_context ioContext;
  const auto endpoint = resolveEndpoint(ioContext, host, port);

  AsioLineServer server(ioContext, endpoint, handler);
  server.startAccept();

  std::cout << "SimpleElo server listening on " << host << ":" << port << std::endl;
  ioContext.run();
}

}  // namespace simpleelo::server::net