#include "client/app/pages.h"

#include <imgui.h>

#include "client/components/profileSections.h"

namespace simpleelo::client::app {

void renderProfilePage(simpleelo::client::ui::AppState& state, const RenderContext& ctx) {
  beginStandardPageWindow("Profile", ctx);
  beginCenteredContentColumn("ProfilePageColumn", 980.0f);

  components::renderProfileSections(state);

  endCenteredContentColumn();
  ImGui::End();
}

}  // namespace simpleelo::client::app
