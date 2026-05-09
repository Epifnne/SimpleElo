#include "client/net/httpClient.h"

#include <stdexcept>
#include <string>
#include <utility>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#endif

namespace simpleelo::client::net {

namespace {

std::runtime_error makeWsaError(const char* stage) {
  const int code = static_cast<int>(WSAGetLastError());
  return std::runtime_error(std::string(stage) + " failed (WSA=" + std::to_string(code) + ")");
}

}  // namespace

std::string sendJsonLine(const std::string& host, int port, const std::string& payload) {
#if defined(_WIN32)
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
    throw std::runtime_error("WSAStartup failed");
  }

  SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock == INVALID_SOCKET) {
    WSACleanup();
    throw std::runtime_error("socket create failed");
  }

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(static_cast<u_short>(port));
  if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
    closesocket(sock);
    WSACleanup();
    throw std::runtime_error("invalid IPv4 address: " + host);
  }

  if (connect(sock, reinterpret_cast<SOCKADDR*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
    closesocket(sock);
    WSACleanup();
    throw makeWsaError("connect");
  }

  const std::string line = payload + "\n";
  if (send(sock, line.c_str(), static_cast<int>(line.size()), 0) == SOCKET_ERROR) {
    closesocket(sock);
    WSACleanup();
    throw makeWsaError("send");
  }

  std::string response;
  char buffer[2048];
  while (true) {
    const int received = recv(sock, buffer, sizeof(buffer), 0);
    if (received == SOCKET_ERROR) {
      closesocket(sock);
      WSACleanup();
      throw makeWsaError("recv");
    }
    if (received <= 0) {
      break;
    }
    response.append(buffer, buffer + received);
    if (response.find('\n') != std::string::npos) {
      break;
    }
  }

  closesocket(sock);
  WSACleanup();

  const auto pos = response.find('\n');
  if (pos != std::string::npos) {
    response = response.substr(0, pos);
  }
  if (response.empty()) {
    throw std::runtime_error("empty response from server");
  }
  return response;
#else
  (void)host;
  (void)port;
  (void)payload;
  throw std::runtime_error("Windows only demo");
#endif
}

}  // namespace simpleelo::client::net
