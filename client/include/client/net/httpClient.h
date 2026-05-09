#pragma once

#include <string>

namespace simpleelo::client::net {

std::string sendJsonLine(const std::string& host, int port, const std::string& payload);

}  // namespace simpleelo::client::net
