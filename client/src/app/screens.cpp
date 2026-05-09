#include "client/app/screens.h"

#include <imgui.h>
#include "client/app/pages.h"

namespace simpleelo::client::app {
namespace {

int gWidth = 1080;
int gHeight = 720;

}  // namespace

void setViewportSize(int width, int height) {
  gWidth = width;
  gHeight = height;
}

void renderFrame(simpleelo::client::ui::AppState& state) {
  pumpAsyncRequests(state);

  RenderContext ctx;
  ctx.width = gWidth;
  ctx.height = gHeight;

  if (!state.serverConnected && state.currentPage != static_cast<int>(ClientPage::Connect)) {
    state.currentPage = static_cast<int>(ClientPage::Connect);
  }
  if (state.serverConnected && !state.loggedIn && state.currentPage != static_cast<int>(ClientPage::Connect) &&
      state.currentPage != static_cast<int>(ClientPage::Auth)) {
    state.currentPage = static_cast<int>(ClientPage::Auth);
  }

  const auto page = static_cast<ClientPage>(state.currentPage);
  switch (page) {
    case ClientPage::Connect:
      renderConnectPage(state, ctx);
      break;
    case ClientPage::Auth:
      renderAuthPage(state, ctx);
      break;
    case ClientPage::Lobby:
      renderLobbyPage(state, ctx);
      break;
    case ClientPage::Room:
      renderRoomPage(state, ctx);
      break;
    case ClientPage::Profile:
      renderProfilePage(state, ctx);
      break;
    case ClientPage::UserInspect:
      renderUserInspectPage(state, ctx);
      break;
    default:
      state.currentPage = static_cast<int>(ClientPage::Connect);
      renderConnectPage(state, ctx);
      break;
  }

  if (state.networkTimeoutPopupOpen) {
    ImGui::OpenPopup("Network Timeout");
    state.networkTimeoutPopupOpen = false;
  }
  if (ImGui::BeginPopupModal("Network Timeout", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::TextWrapped("%s", state.networkTimeoutPopupMessage.empty() ? "网络连接超时" : state.networkTimeoutPopupMessage.c_str());
    if (ImGui::Button("OK")) {
      state.networkTimeoutPopupMessage.clear();
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

}  // namespace simpleelo::client::app
