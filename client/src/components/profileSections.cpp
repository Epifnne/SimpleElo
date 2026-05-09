#include "client/components/profileSections.h"

#include <array>
#include <string>
#include <ctime>

#include <imgui.h>

#include "client/app/pages.h"
#include "client/uiCommon.h"
#include "protocol/api.h"

namespace simpleelo::client::app::components {

namespace {

std::string formatEpoch(std::int64_t epochSec) {
  if (epochSec <= 0) {
    return "-";
  }
  std::time_t t = static_cast<std::time_t>(epochSec);
  std::tm tmValue{};
#if defined(_WIN32)
  localtime_s(&tmValue, &t);
#else
  localtime_r(&t, &tmValue);
#endif
  std::array<char, 32> buf{};
  if (std::strftime(buf.data(), buf.size(), "%Y-%m-%d %H:%M", &tmValue) == 0) {
    return "-";
  }
  return buf.data();
}

void loadProfileViewData(simpleelo::client::ui::AppState& state) {
  refreshProfile(state);
  (void)startAsyncRequest(state, "profileGetHistory", protocol::api::buildGetUserHistoryRequest(state.token, state.selfUserId));
}

void loadMatchDetail(simpleelo::client::ui::AppState& state, const std::string& matchId) {
  state.historySelectedMatchId = matchId;
  (void)startAsyncRequest(state, "profileMatchDetail", protocol::api::buildGetMatchRequest(state.token, matchId));
}

void renderMatchTeams(const char* title, const nlohmann::json& arr) {
  ImGui::TextUnformatted(title);
  ImGui::BeginChild(title, ImVec2(0.0f, 120.0f), true);
  if (arr.is_array()) {
    for (const auto& row : arr) {
      ImGui::Text("%s | %s (%d)",
                  row.value("nickname", std::string("player")).c_str(),
                  row.value("role", std::string("-")).c_str(),
                  row.value("roleScore", 0));
    }
  }
  ImGui::EndChild();
}

}  // namespace

void renderProfileSections(simpleelo::client::ui::AppState& state) {
  nlohmann::json asyncResp;
  if (tryTakeAsyncResponse(state, "profileGetHistory", asyncResp)) {
    if (asyncResp.value("code", -1) == 0) {
      state.profileHistoryPayload = asyncResp;
    }
  }
  if (tryTakeAsyncResponse(state, "profileMatchDetail", asyncResp)) {
    if (asyncResp.value("code", -1) == 0 && asyncResp.contains("match") && asyncResp["match"].is_object()) {
      state.historySelectedMatchDetail = asyncResp["match"];
    }
  }

  if (!state.loggedIn) {
    ImGui::TextWrapped("请先登录后查看个人信息。");
    if (ImGui::Button("Go To Login/Register")) {
      state.currentPage = static_cast<int>(ClientPage::Auth);
    }
    return;
  }

  if (!state.profileAutoLoadDone || state.profileAutoLoadUserId != state.selfUserId) {
    loadProfileViewData(state);
    state.profileAutoLoadDone = true;
    state.profileAutoLoadUserId = state.selfUserId;
  }

  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.22f, 0.22f, 0.22f, 1.0f));
  ImGui::Text("Target: %s", state.selfNickname.c_str());
  ImGui::PopStyleColor();
  if (ImGui::Button("Reload")) {
    loadProfileViewData(state);
    state.profileAutoLoadDone = true;
    state.profileAutoLoadUserId = state.selfUserId;
  }
  ImGui::SameLine();
  if (ImGui::Button(state.profileEditMode ? "Close Edit" : "Edit")) {
    state.profileEditMode = !state.profileEditMode;
  }
  ImGui::SameLine();
  if (ImGui::Button("Back")) {
    state.currentPage = static_cast<int>(ClientPage::Lobby);
  }

  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.985f, 0.985f, 0.98f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.88f, 0.88f, 0.86f, 1.0f));
  ImGui::BeginChild("ProfileSummary", ImVec2(0.0f, 106.0f), true);
  if (state.profileHistoryPayload.contains("user") && state.profileHistoryPayload["user"].is_object()) {
    const auto& user = state.profileHistoryPayload["user"];
    const int wins = user.value("wins", 0);
    const int losses = user.value("losses", 0);
    const int total = wins + losses;
    const float rate = total > 0 ? (static_cast<float>(wins) * 100.0f / static_cast<float>(total)) : 0.0f;
    ImGui::Text("UserId: %lld", static_cast<long long>(user.value("userId", state.selfUserId)));
    ImGui::Text("ELO: %d", user.value("elo", state.selfElo));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.42f, 0.56f, 1.0f));
    ImGui::Text("WinRate: %.1f%% (%d/%d)", rate, wins, total);
    ImGui::PopStyleColor();
  } else {
    ImGui::Text("UserId: %lld", static_cast<long long>(state.selfUserId));
    ImGui::Text("ELO: %d", state.selfElo);
    ImGui::TextWrapped("正在加载和他人页面一致的历史数据视图。若失败可点 Reload 重试。");
  }
  ImGui::EndChild();
  ImGui::PopStyleColor(2);

  if (state.profileEditMode) {
    ImGui::SeparatorText("Edit");
    ImGui::InputText("New Nickname", state.profileNicknameDraft.data(), state.profileNicknameDraft.size());
    ImGui::InputText("New Password", state.profilePasswordDraft.data(), state.profilePasswordDraft.size());
    ImGui::InputText("Confirm Password", state.profilePasswordConfirmDraft.data(), state.profilePasswordConfirmDraft.size());

    if (ImGui::Button("Apply (Local Placeholder)")) {
      if (state.profileNicknameDraft[0] != '\0') {
        state.selfNickname = state.profileNicknameDraft.data();
      }
      simpleelo::client::ui::appendLog(state.logs, "profile update UI is ready; server API not wired yet.");
      state.lastResponse = "{\n  \"code\": 0,\n  \"message\": \"profile API not implemented on server yet\"\n}";
      state.profileEditMode = false;
    }
  }

  ImGui::SeparatorText("Match History");
  ImGui::BeginChild("ProfileHistoryList", ImVec2(0.0f, 200.0f), true);
  if (state.profileHistoryPayload.contains("history") && state.profileHistoryPayload["history"].is_array()) {
    for (const auto& h : state.profileHistoryPayload["history"]) {
      const std::string mid = h.value("matchId", "-");
      const std::string role = h.value("role", "-");
      const std::string dateText = formatEpoch(h.value("createdAtEpochSec", 0LL));
      const int delta = h.value("delta", 0);
      const int before = h.value("eloBefore", 0);
      const int after = h.value("eloAfter", 0);
      const std::string outcome = h.value("outcome", "-");
      const bool selected = state.historySelectedMatchId == mid;
      const std::string rowLabel = mid + " | " + dateText + " | role:" + role + " | " + outcome + " | " +
                                   std::to_string(before) + " -> " + std::to_string(after) + " (" +
                                   (delta >= 0 ? "+" : "") + std::to_string(delta) + ")";
      if (ImGui::Selectable(rowLabel.c_str(), selected)) {
        loadMatchDetail(state, mid);
      }
    }
  } else {
    ImGui::TextWrapped("暂无历史数据。若网络请求失败可点 Reload 重试。");
  }
  ImGui::EndChild();

  if (state.historySelectedMatchDetail.is_object()) {
    ImGui::SeparatorText("Match Detail");
    ImGui::Text("MatchId: %s", state.historySelectedMatchDetail.value("matchId", std::string("-")).c_str());
    ImGui::Text("Winner: %s", state.historySelectedMatchDetail.value("winner", std::string("-")).c_str());
    ImGui::Text("Date: %s", formatEpoch(state.historySelectedMatchDetail.value("createdAtEpochSec", 0LL)).c_str());

    ImGui::Columns(2, "ProfileMatchTeams", true);
    renderMatchTeams("Team Red", state.historySelectedMatchDetail.value("teamRed", nlohmann::json::array()));
    ImGui::NextColumn();
    renderMatchTeams("Team Blue", state.historySelectedMatchDetail.value("teamBlue", nlohmann::json::array()));
    ImGui::Columns(1);
  }
}

}  // namespace simpleelo::client::app::components
