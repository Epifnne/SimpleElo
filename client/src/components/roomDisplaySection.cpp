#include "client/components/roomDisplaySection.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

#include <imgui.h>

#include "client/app/pages.h"
#include "client/uiCommon.h"
#include "protocol/api.h"

namespace simpleelo::client::app::components {
namespace {

void openUserInspectFromRoom(simpleelo::client::ui::AppState& state, const nlohmann::json& row) {
  const std::int64_t uid = row.value("userId", 0LL);
  if (uid == state.selfUserId) {
    state.profileAutoLoadDone = false;
    state.currentPage = static_cast<int>(ClientPage::Profile);
    return;
  }

  const std::string name = row.value("nickname", "player");
  state.inspectPendingTargetName = name;
  state.inspectPendingReturnPage = static_cast<int>(ClientPage::Room);
  (void)startAsyncRequest(state, "inspectUserHistory", protocol::api::buildGetUserHistoryRequest(state.token, uid));
}

void renderRoomPlayerRow(simpleelo::client::ui::AppState& state, const nlohmann::json& row, const char* sideTag) {
  const std::int64_t uid = row.value("userId", 0LL);
  const std::string name = row.value("nickname", "player");
  const int elo = row.value("elo", 1000);
  const std::string role = row.value("role", "-");
  const int roleScore = row.value("roleScore", 0);
  const bool ready = row.value("ready", false);
  const bool online = row.value("online", true);
  const bool leftEarly = row.value("leftEarly", false);
  const int winDelta = row.value("expectedWinDelta", 0);
  const int loseDelta = row.value("expectedLoseDelta", 0);

  ImGui::PushID(static_cast<int>(uid));
  const std::string avatar = name.empty() ? "?" : std::string(1, static_cast<char>(std::toupper(static_cast<unsigned char>(name[0]))));
  if (ImGui::SmallButton(avatar.c_str())) {
    openUserInspectFromRoom(state, row);
  }
  ImGui::SameLine();
  if (!online || leftEarly) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.55f, 1.0f));
  }
  ImGui::Text("%s | %s(%d)%s", sideTag, name.c_str(), elo, ready ? " [READY]" : " [NOT READY]");
  ImGui::SameLine();
  ImGui::Text("Role: %s (%d)", role.c_str(), roleScore);
  ImGui::SameLine();
  ImGui::Text("ELO W/L: %+d / %+d", winDelta, loseDelta);
  if (!online || leftEarly) {
    ImGui::PopStyleColor();
  }
  ImGui::PopID();
}

std::vector<std::string> buildRoleOptions(const simpleelo::client::ui::AppState& state) {
  std::vector<std::string> options;
  auto pushUnique = [&](const std::string& value) {
    if (value.empty()) {
      return;
    }
    if (std::find(options.begin(), options.end(), value) == options.end()) {
      options.push_back(value);
    }
  };

  pushUnique("captain");
  pushUnique(state.selfRole.data());
  pushUnique(state.roomDraftRole.data());

  auto collectFrom = [&](const nlohmann::json& arr) {
    if (!arr.is_array()) {
      return;
    }
    for (const auto& row : arr) {
      pushUnique(row.value("role", std::string{}));
    }
  };

  if (state.roomDetail.contains("teamRed")) {
    collectFrom(state.roomDetail["teamRed"]);
  }
  if (state.roomDetail.contains("teamBlue")) {
    collectFrom(state.roomDetail["teamBlue"]);
  }

  return options;
}

}  // namespace

void renderRoomDisplaySection(simpleelo::client::ui::AppState& state,
                              bool isOwner,
                              const std::function<void()>& reloadRoomDetail) {
  nlohmann::json asyncResp;
  if (tryTakeAsyncResponse(state, "inspectUserHistory", asyncResp)) {
    if (asyncResp.value("code", -1) == 0) {
      state.showHistoryPopup = true;
      state.historyTargetName = state.inspectPendingTargetName;
      state.historyPayload = asyncResp;
      state.inspectReturnPage = state.inspectPendingReturnPage;
      state.currentPage = static_cast<int>(ClientPage::UserInspect);
    }
  }
  if (tryTakeAsyncResponse(state, "roomLeave", asyncResp)) {
    if (asyncResp.value("code", -1) == 0) {
      state.selectedRoomId[0] = '\0';
      state.roomDetail = nlohmann::json{};
      state.currentPage = static_cast<int>(ClientPage::Lobby);
      refreshRooms(state);
    }
  }
  if (tryTakeAsyncResponse(state, "roomSetBp", asyncResp)) {
    reloadRoomDetail();
  }
  if (tryTakeAsyncResponse(state, "roomStartGame", asyncResp)) {
    reloadRoomDetail();
  }
  if (tryTakeAsyncResponse(state, "roomAbortGame", asyncResp)) {
    reloadRoomDetail();
  }
  if (tryTakeAsyncResponse(state, "roomDeleteRoom", asyncResp)) {
    if (asyncResp.value("code", -1) == 0) {
      state.globalNotice = "room has been deleted";
      state.globalNoticeUntil = ImGui::GetTime() + 4.0;
      state.selectedRoomId[0] = '\0';
      state.roomDetail = nlohmann::json{};
      state.currentPage = static_cast<int>(ClientPage::Lobby);
      refreshRooms(state);
      return;
    }
  }
  if (tryTakeAsyncResponse(state, "roomSetPlayer", asyncResp)) {
    state.roomSlotEditPending = false;
    reloadRoomDetail();
  }

  if (!state.globalNotice.empty() && ImGui::GetTime() < state.globalNoticeUntil) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.75f, 0.25f, 1.0f));
    ImGui::TextWrapped("%s", state.globalNotice.c_str());
    ImGui::PopStyleColor();
  }

  ImGui::Text("Room: %s", state.selectedRoomId.data());
  ImGui::SameLine();
  if (ImGui::Button("Refresh Room")) {
    reloadRoomDetail();
  }
  ImGui::SameLine();
  if (ImGui::Button("Profile")) {
    state.currentPage = static_cast<int>(ClientPage::Profile);
  }
  ImGui::SameLine();
  if (ImGui::Button("Leave Room")) {
    (void)startAsyncRequest(state, "roomLeave", protocol::api::buildLeaveRoomRequest(state.token, state.selectedRoomId.data()));
  }

  const bool hasRoom = state.roomDetail.contains("room") && state.roomDetail["room"].is_object();
  const bool locked = hasRoom && state.roomDetail["room"].value("locked", false);
  const bool gameStarted = hasRoom && state.roomDetail["room"].value("gameStarted", false);
  const bool bpEnabled = hasRoom && state.roomDetail["room"].value("bpEnabled", false);

  ImGui::Text("Room Lock: %s", locked ? "ON" : "OFF");
  ImGui::SameLine();
  ImGui::Text("Game: %s", gameStarted ? "STARTED" : "WAITING");
  ImGui::SameLine();
  ImGui::Text("BP: %s", bpEnabled ? "ON" : "OFF");
  ImGui::SameLine();
  ImGui::Text("Owner: %s", isOwner ? "ME" : "OTHER");

  if (isOwner) {
    ImGui::SeparatorText("Owner Controls");
    bool bpDraft = bpEnabled;
    if (ImGui::Checkbox("Enable BP", &bpDraft)) {
      (void)startAsyncRequest(state,
                              "roomSetBp",
                              protocol::api::buildSetRoomBpRequest(state.token, state.selectedRoomId.data(), bpDraft));
    }
    if (ImGui::Button("Start Game (Lock Room)")) {
      (void)startAsyncRequest(state,
                              "roomStartGame",
                              protocol::api::buildStartGameRequest(state.token, state.selectedRoomId.data()));
    }
    ImGui::SameLine();
    if (ImGui::Button("Abort Game (No ELO)")) {
      (void)startAsyncRequest(state,
                              "roomAbortGame",
                              protocol::api::buildAbortGameRequest(state.token, state.selectedRoomId.data()));
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete Room")) {
      (void)startAsyncRequest(state,
                              "roomDeleteRoom",
                              protocol::api::buildDeleteRoomRequest(state.token, state.selectedRoomId.data()));
    }
  }

  const char* teams[] = {"red", "blue"};
  ImGui::SeparatorText("Role / Team");
  if (ImGui::Combo("Team", &state.roomDraftTeamIndex, teams, IM_ARRAYSIZE(teams))) {
    state.roomSlotEditPending = true;
  }
  const auto roleOptions = buildRoleOptions(state);
  const char* rolePreview = state.roomDraftRole[0] == '\0' ? "(select role)" : state.roomDraftRole.data();
  if (ImGui::BeginCombo("Role", rolePreview)) {
    for (const auto& role : roleOptions) {
      const bool selected = role == std::string(state.roomDraftRole.data());
      if (ImGui::Selectable(role.c_str(), selected)) {
        simpleelo::client::ui::copyToBuffer(state.roomDraftRole.data(), state.roomDraftRole.size(), role);
        state.roomSlotEditPending = true;
      }
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }

  if (std::string(state.roomDraftRole.data()) == std::string(state.selfRole.data())) {
    ImGui::Text("RoleScore (from server): %d", state.selfRoleScore);
  } else {
    ImGui::TextWrapped("RoleScore 由服务器返回；选择新 role 后切换 Ready 触发提交并等待刷新。当前 server roleScore: %d", state.selfRoleScore);
  }

  if (ImGui::Checkbox("Ready", &state.roomDraftReady)) {
    state.roomSlotEditPending = true;
    (void)startAsyncRequest(state,
                            "roomSetPlayer",
                            {{"action", protocol::api::kActionSetPlayer},
                             {"token", state.token},
                             {"roomId", state.selectedRoomId.data()},
                             {"team", teams[state.roomDraftTeamIndex]},
                             {"role", state.roomDraftRole.data()},
                             {"roleScore", state.selfRoleScore},
                             {"ready", state.roomDraftReady},
                             {"leftEarly", state.selfLeftEarly}});
  }

  const bool hasDraftChanges =
      state.roomDraftTeamIndex != state.selfTeamIndex ||
      std::string(state.roomDraftRole.data()) != std::string(state.selfRole.data()) ||
      state.roomDraftReady != state.selfReady;
  if (hasDraftChanges) {
    ImGui::TextWrapped("已修改 Team/Role，勾选或取消 Ready 会直接提交。\nRoleScore 由服务端计算并在刷新后更新。");
  }

  ImGui::SeparatorText("Room Board");
  ImGui::Columns(2, "TeamColumns", true);
  ImGui::Text("Team 1 (Red)");
  if (state.roomDetail.contains("teamRed") && state.roomDetail["teamRed"].is_array()) {
    for (const auto& row : state.roomDetail["teamRed"]) {
      renderRoomPlayerRow(state, row, "R");
    }
  }
  ImGui::NextColumn();
  ImGui::Text("Team 2 (Blue)");
  if (state.roomDetail.contains("teamBlue") && state.roomDetail["teamBlue"].is_array()) {
    for (const auto& row : state.roomDetail["teamBlue"]) {
      renderRoomPlayerRow(state, row, "B");
    }
  }
  ImGui::Columns(1);
}

}  // namespace simpleelo::client::app::components
