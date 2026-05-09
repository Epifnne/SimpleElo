#pragma once

#include <string>

#include "client/ui/appState.h"

namespace simpleelo::client::app::components {

bool renderConnectActionSection(simpleelo::client::ui::AppState& state);
void renderConnectStatusSection(const std::string& message, bool isError, bool connected, const char* host, int port);

}  // namespace simpleelo::client::app::components
