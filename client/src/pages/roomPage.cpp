#include "client/app/pages.h"

#include <imgui.h>

#include "client/components/roomSections.h"

namespace simpleelo::client::app {

void renderRoomPage(simpleelo::client::ui::AppState& state, const RenderContext& ctx) {
  beginStandardPageWindow("Room", ctx);
  beginCenteredContentColumn("RoomPageColumn", 1080.0f);
  components::renderRoomSections(state);

  endCenteredContentColumn();
  ImGui::End();
}

}  // namespace simpleelo::client::app
