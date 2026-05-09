#include "client/components/lobbySections.h"

#include <algorithm>
#include <cctype>
#include <string>

#include <imgui.h>

#include "client/app/pages.h"
#include "client/uiCommon.h"
#include "protocol/api.h"

namespace simpleelo::client::app::components {
namespace {

std::string toLowerCopy(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return s;
}

bool roomMatchesKeyword(const nlohmann::json& room, const std::string& keywordLower) {
  if (keywordLower.empty()) {
    return true;
  }
  const std::string roomId = toLowerCopy(room.value("roomId", ""));
  const std::string roomName = toLowerCopy(room.value("roomName", ""));
  return roomId.find(keywordLower) != std::string::npos || roomName.find(keywordLower) != std::string::npos;
}

void openUserInspectFromLobby(simpleelo::client::ui::AppState& state, const nlohmann::json& row) {
  const std::int64_t uid = row.value("userId", 0LL);
  const std::string name = row.value("nickname", "player");
  state.inspectPendingTargetName = name;
  state.inspectPendingReturnPage = static_cast<int>(ClientPage::Lobby);
  (void)startAsyncRequest(state, "inspectUserHistory", protocol::api::buildGetUserHistoryRequest(state.token, uid));
}

}  // namespace

void renderLobbySections(simpleelo::client::ui::AppState& state) {
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
  if (tryTakeAsyncResponse(state, "lobbyCreateRoom", asyncResp)) {
    if (asyncResp.value("code", -1) == 0) {
      simpleelo::client::ui::copyToBuffer(state.selectedRoomId.data(), state.selectedRoomId.size(), asyncResp.value("roomId", ""));
      refreshRooms(state);
      refreshRoomDetail(state);
      state.currentPage = static_cast<int>(ClientPage::Room);
    }
  }
  if (tryTakeAsyncResponse(state, "lobbyJoinRoom", asyncResp)) {
    if (asyncResp.value("code", -1) == 0) {
      refreshRoomDetail(state);
      refreshRooms(state);
      state.currentPage = static_cast<int>(ClientPage::Room);
    } else if (asyncResp.value("code", -1) == 438) {
      state.globalNotice = asyncResp.value("message", std::string("room has been deleted"));
      state.globalNoticeUntil = ImGui::GetTime() + 4.0;
      state.selectedRoomId[0] = '\0';
      refreshRooms(state);
    }
  }

  if (!state.loggedIn) {
    ImGui::TextWrapped("请先登录。此页面用于创建房间和检索房间。");
    if (ImGui::Button("Go To Login/Register")) {
      state.currentPage = static_cast<int>(ClientPage::Auth);
    }
    return;
  }

  if (!state.globalNotice.empty() && ImGui::GetTime() < state.globalNoticeUntil) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.56f, 0.44f, 0.20f, 1.0f));
    ImGui::TextWrapped("%s", state.globalNotice.c_str());
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0.0f, 2.0f));
  }

  ImGui::Text("%s | ELO %d", state.selfNickname.c_str(), state.selfElo);
  ImGui::Dummy(ImVec2(0.0f, 2.0f));
  ImGui::SameLine();
  if (ImGui::Button("Refresh Profile")) {
    refreshProfile(state);
  }
  ImGui::SameLine();
  if (ImGui::Button("Refresh Rooms")) {
    refreshRooms(state);
  }
  ImGui::SameLine();
  if (ImGui::Button("Profile")) {
    state.currentPage = static_cast<int>(ClientPage::Profile);
  }
  ImGui::SameLine();
  if (ImGui::Button("Logout")) {
    state.loggedIn = false;
    state.token.clear();
    state.profileHistoryPayload = nlohmann::json{};
    state.profileEditMode = false;
    state.profileAutoLoadDone = false;
    state.profileAutoLoadUserId = 0;
    state.currentPage = static_cast<int>(ClientPage::Auth);
  }

  ImGui::Separator();

  if (ImGui::Button("Create Room")) {
    ImGui::OpenPopup("CreateRoomPopup");
  }
  if (ImGui::BeginPopupModal("CreateRoomPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::InputText("Room Name", state.createRoomName.data(), state.createRoomName.size());
    ImGui::InputText("Room Password", state.createRoomPassword.data(), state.createRoomPassword.size());
    if (ImGui::Button("Confirm Create")) {
      const protocol::api::CreateRoomRequest request{state.token, state.createRoomName.data(), state.createRoomPassword.data()};
      if (!handleValidationFailure(state, protocol::api::validateCreateRoomRequest(request), "createRoom")) {
        (void)startAsyncRequest(state, "lobbyCreateRoom", protocol::api::buildCreateRoomRequest(request));
        ImGui::CloseCurrentPopup();
      }
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  ImGui::SeparatorText("Room Search");
  ImGui::InputText("Keyword (name/id)", state.lobbyRoomKeyword.data(), state.lobbyRoomKeyword.size());
  ImGui::Checkbox("Only Joinable (not lock)", &state.lobbyOnlyJoinable);

  const std::string keywordLower = toLowerCopy(state.lobbyRoomKeyword.data());
  int matchedRooms = 0;
  for (const auto& room : state.roomList) {
    const bool hasPassword = room.value("hasPassword", false);
    if (!roomMatchesKeyword(room, keywordLower)) {
      continue;
    }
    if (state.lobbyOnlyJoinable && hasPassword) {
      continue;
    }
    ++matchedRooms;
  }
  ImGui::Text("Matched: %d / %d", matchedRooms, static_cast<int>(state.roomList.size()));

  ImGui::BeginChild("RoomListArea", ImVec2(0.0f, 240.0f), true);
  for (const auto& room : state.roomList) {
    const std::string roomId = room.value("roomId", "");
    const std::string roomName = room.value("roomName", "-");
    const int memberCount = room.value("memberCount", 0);
    const bool hasPassword = room.value("hasPassword", false);
    if (!roomMatchesKeyword(room, keywordLower)) {
      continue;
    }
    if (state.lobbyOnlyJoinable && hasPassword) {
      continue;
    }
    const bool selected = std::string(state.selectedRoomId.data()) == roomId;
    std::string label = roomName + "  [" + roomId + "]  members:" + std::to_string(memberCount) + (hasPassword ? "  lock" : "  open");
    if (ImGui::Selectable(label.c_str(), selected)) {
      simpleelo::client::ui::copyToBuffer(state.selectedRoomId.data(), state.selectedRoomId.size(), roomId);
      refreshRoomDetail(state);
    }
  }
  ImGui::EndChild();

  ImGui::InputText("Selected Room", state.selectedRoomId.data(), state.selectedRoomId.size());
  ImGui::InputText("Join Password", state.joinRoomPassword.data(), state.joinRoomPassword.size());

  if (ImGui::Button("Join Selected")) {
    const protocol::api::JoinRoomRequest request{state.token, state.selectedRoomId.data(), state.joinRoomPassword.data()};
    if (!handleValidationFailure(state, protocol::api::validateJoinRoomRequest(request), "joinRoom")) {
      (void)startAsyncRequest(state, "lobbyJoinRoom", protocol::api::buildJoinRoomRequest(request));
    }
  }

  ImGui::SeparatorText("Inspect Players (Selected Room)");
  if (ImGui::Button("Load Selected Room Players")) {
    refreshRoomDetail(state);
  }
  if (state.roomDetail.contains("teamRed") && state.roomDetail["teamRed"].is_array()) {
    for (const auto& row : state.roomDetail["teamRed"]) {
      const std::string name = row.value("nickname", "player");
      const std::string avatar = name.empty() ? "?" : std::string(1, static_cast<char>(std::toupper(static_cast<unsigned char>(name[0]))));
      ImGui::PushID(static_cast<int>(row.value("userId", 0LL)));
      if (ImGui::SmallButton(avatar.c_str())) {
        openUserInspectFromLobby(state, row);
      }
      ImGui::SameLine();
      ImGui::Text("R | %s", name.c_str());
      ImGui::PopID();
    }
  }
  if (state.roomDetail.contains("teamBlue") && state.roomDetail["teamBlue"].is_array()) {
    for (const auto& row : state.roomDetail["teamBlue"]) {
      const std::string name = row.value("nickname", "player");
      const std::string avatar = name.empty() ? "?" : std::string(1, static_cast<char>(std::toupper(static_cast<unsigned char>(name[0]))));
      ImGui::PushID(100000 + static_cast<int>(row.value("userId", 0LL)));
      if (ImGui::SmallButton(avatar.c_str())) {
        openUserInspectFromLobby(state, row);
      }
      ImGui::SameLine();
      ImGui::Text("B | %s", name.c_str());
      ImGui::PopID();
    }
  }

}

}  // namespace simpleelo::client::app::components
