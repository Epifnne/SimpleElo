#include "client/components/roomSections.h"

#include <string>

#include <imgui.h>

#include "client/app/pages.h"
#include "client/components/roomDisplaySection.h"
#include "client/components/roomResultSection.h"
#include "client/uiCommon.h"
#include "protocol/api.h"

namespace simpleelo::client::app::components {
namespace {

bool syncSelfFromRoomDetail(simpleelo::client::ui::AppState& state) {
  auto syncFromArray = [&](const nlohmann::json& arr, int teamIndex) {
    if (!arr.is_array()) {
      return false;
    }
    for (const auto& row : arr) {
      if (row.value("userId", 0LL) != state.selfUserId) {
        continue;
      }
      state.selfTeamIndex = teamIndex;
      simpleelo::client::ui::copyToBuffer(state.selfRole.data(), state.selfRole.size(), row.value("role", std::string{}));
      state.selfRoleScore = row.value("roleScore", 0);
      state.selfReady = row.value("ready", false);
      state.selfLeftEarly = row.value("leftEarly", false);
      if (!state.roomSlotEditPending) {
        state.roomDraftTeamIndex = state.selfTeamIndex;
        simpleelo::client::ui::copyToBuffer(state.roomDraftRole.data(), state.roomDraftRole.size(), state.selfRole.data());
        state.roomDraftRoleScore = state.selfRoleScore;
        state.roomDraftReady = state.selfReady;
      }
      return true;
    }
    return false;
  };

  if (state.roomDetail.contains("teamRed") && syncFromArray(state.roomDetail["teamRed"], 0)) {
    return true;
  }
  if (state.roomDetail.contains("teamBlue") && syncFromArray(state.roomDetail["teamBlue"], 1)) {
    return true;
  }
  return false;
}

}  // namespace

void renderRoomSections(simpleelo::client::ui::AppState& state) {
  static double sNextRoomPollAt = 0.0;
  auto reloadRoomDetail = [&]() {
    refreshRoomDetail(state);
  };

  const double now = ImGui::GetTime();
  if (now >= sNextRoomPollAt && !std::string(state.selectedRoomId.data()).empty()) {
    reloadRoomDetail();
    sNextRoomPollAt = now + 1.5;
  }

  if (!state.loggedIn) {
    ImGui::TextWrapped("请先登录。此页面提供离开房间、角色选择和房间信息查看。");
    if (ImGui::Button("Go To Login/Register")) {
      state.currentPage = static_cast<int>(ClientPage::Auth);
    }
    return;
  }

  if (std::string(state.selectedRoomId.data()).empty()) {
    ImGui::TextWrapped("当前未进入房间，请先在主界面选择或加入房间。");
    if (ImGui::Button("Go To Main Lobby")) {
      state.currentPage = static_cast<int>(ClientPage::Lobby);
    }
    return;
  }

  const bool hasRoom = state.roomDetail.contains("room") && state.roomDetail["room"].is_object();
  const std::int64_t ownerUserId = hasRoom ? state.roomDetail["room"].value("ownerUserId", 0LL) : 0LL;
  const bool isOwner = ownerUserId == state.selfUserId;

  (void)syncSelfFromRoomDetail(state);

  renderRoomDisplaySection(state, isOwner, reloadRoomDetail);
  if (state.currentPage != static_cast<int>(ClientPage::Room)) {
    return;
  }
  renderRoomResultSection(state, isOwner);
}

}  // namespace simpleelo::client::app::components
