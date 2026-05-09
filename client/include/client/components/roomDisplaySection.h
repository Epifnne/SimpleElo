#pragma once

#include <functional>

#include "client/ui/appState.h"

namespace simpleelo::client::app::components {

void renderRoomDisplaySection(simpleelo::client::ui::AppState& state,
                              bool isOwner,
                              const std::function<void()>& reloadRoomDetail);

}  // namespace simpleelo::client::app::components
