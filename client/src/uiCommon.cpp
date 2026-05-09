#include "client/uiCommon.h"

#include <cstring>

namespace simpleelo::client::ui {

void appendLog(std::vector<std::string>& logs, const std::string& message) {
  logs.push_back(message);
  if (logs.size() > 200) {
    logs.erase(logs.begin(), logs.begin() + static_cast<long long>(logs.size() - 200));
  }
}

void copyToBuffer(char* buffer, size_t bufferSize, const std::string& value) {
  if (bufferSize == 0) {
    return;
  }
  std::strncpy(buffer, value.c_str(), bufferSize - 1);
  buffer[bufferSize - 1] = '\0';
}

}  // namespace simpleelo::client::ui
