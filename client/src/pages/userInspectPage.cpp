#include "client/app/pages.h"

#include <imgui.h>

#include "client/components/userInspectSections.h"

namespace simpleelo::client::app {

void renderUserInspectPage(simpleelo::client::ui::AppState& state, const RenderContext& ctx) {
  beginStandardPageWindow("User Inspect", ctx);
  beginCenteredContentColumn("UserInspectPageColumn", 1020.0f);

  components::renderUserInspectSections(state);

  endCenteredContentColumn();
  ImGui::End();
}

}  // namespace simpleelo::client::app
