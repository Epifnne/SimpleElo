#pragma once

#include "client/app/pageHelpers.h"

namespace simpleelo::client::app {

enum class ClientPage : int {
  Connect = 0,
  Auth = 1,
  Lobby = 2,
  Room = 3,
  Profile = 4,
  UserInspect = 5,
};

void renderConnectPage(simpleelo::client::ui::AppState& state, const RenderContext& ctx);
void renderAuthPage(simpleelo::client::ui::AppState& state, const RenderContext& ctx);
void renderLobbyPage(simpleelo::client::ui::AppState& state, const RenderContext& ctx);
void renderRoomPage(simpleelo::client::ui::AppState& state, const RenderContext& ctx);
void renderProfilePage(simpleelo::client::ui::AppState& state, const RenderContext& ctx);
void renderUserInspectPage(simpleelo::client::ui::AppState& state, const RenderContext& ctx);

}  // namespace simpleelo::client::app
