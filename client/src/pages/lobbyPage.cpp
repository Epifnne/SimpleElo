#include "client/app/pages.h"

#include <imgui.h>

#include "client/components/lobbySections.h"

namespace simpleelo::client::app {

void renderLobbyPage(simpleelo::client::ui::AppState& state, const RenderContext& ctx) {
  beginStandardPageWindow("Main Lobby", ctx);
  beginCenteredContentColumn("LobbyPageColumn", 1080.0f);
  components::renderLobbySections(state);

  endCenteredContentColumn();
  ImGui::End();
}

}  // namespace simpleelo::client::app
