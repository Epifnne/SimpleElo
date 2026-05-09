#include "serverEngine.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include "protocol/api.h"

namespace simpleelo::server {
namespace {

using nlohmann::json;

std::string jsonErr(int code, const std::string& message) {
  return json({{"code", code}, {"message", message}}).dump();
}

}  // namespace

ServerEngine::ServerEngine(std::string dataFilePath) : dataFilePath_(std::move(dataFilePath)) {}

std::string ServerEngine::hashPassword(const std::string& password) const {
  const auto hashed = std::hash<std::string>{}(password + "::simpleelo");
  return std::to_string(hashed);
}

std::string ServerEngine::makeToken(std::int64_t userId) const {
  return "token-" + std::to_string(userId) + "-" + std::to_string(nowEpochSec());
}

std::int64_t ServerEngine::verifyToken(const std::string& token) const {
  const auto it = tokenToUserId_.find(token);
  if (it == tokenToUserId_.end()) {
    return 0;
  }
  return it->second;
}

std::int64_t ServerEngine::nowEpochSec() {
  return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string ServerEngine::nowIso8601() {
  const auto now = std::chrono::system_clock::now();
  const std::time_t t = std::chrono::system_clock::to_time_t(now);
  std::tm tmValue{};
#if defined(_WIN32)
  gmtime_s(&tmValue, &t);
#else
  gmtime_r(&t, &tmValue);
#endif
  std::ostringstream oss;
  oss << std::put_time(&tmValue, "%Y-%m-%dT%H:%M:%SZ");
  return oss.str();
}

std::string ServerEngine::handleRequest(const std::string& payload) {
  std::scoped_lock lock(mutex_);
  try {
    const json req = json::parse(payload);
    const std::string action = req.value("action", "");
    return dispatchAction(action, req);
  } catch (const std::exception& ex) {
    return jsonErr(500, std::string("invalid request: ") + ex.what());
  }
}

}  // namespace simpleelo::server
