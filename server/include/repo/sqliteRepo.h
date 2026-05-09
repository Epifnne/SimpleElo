#pragma once

#include <string>

#include <nlohmann/json.hpp>

namespace simpleelo::server::repo {

// Returns true on successful DB access. `found` is true only when state key exists.
bool loadState(const std::string& dbPath, nlohmann::json& state, bool& found);

// Returns true when state is successfully persisted.
bool saveState(const std::string& dbPath, const nlohmann::json& state);

}  // namespace simpleelo::server::repo
