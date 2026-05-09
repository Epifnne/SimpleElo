#pragma once

#include <functional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace simpleelo::client::ui {

using SendRequestFn = std::function<nlohmann::json(const nlohmann::json&)>;

void appendLog(std::vector<std::string>& logs, const std::string& message);
void copyToBuffer(char* buffer, size_t bufferSize, const std::string& value);

}  // namespace simpleelo::client::ui
