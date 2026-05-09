#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace simpleelo::server::net {

using RequestHandler = std::function<std::string(const std::string&)>;

void runAsioLineServer(const std::string& host, std::uint16_t port, const RequestHandler& handler);

}  // namespace simpleelo::server::net