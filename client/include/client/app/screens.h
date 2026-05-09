#pragma once

#include "client/ui/appState.h"

namespace simpleelo::client::app {

void setViewportSize(int width, int height);
void renderFrame(simpleelo::client::ui::AppState& state);

}  // namespace simpleelo::client::app
