#include "client/components/userInspectSections.h"

#include <array>
#include <string>
#include <ctime>

#include <imgui.h>

#include "client/app/pageHelpers.h"
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

void loadMatchDetail(simpleelo::client::ui::AppState& state, const std::string& matchId) {
  state.historySelectedMatchId = matchId;
  (void)startAsyncRequest(state, "inspectMatchDetail", protocol::api::buildGetMatchRequest(state.token, matchId));
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

void renderUserInspectSections(simpleelo::client::ui::AppState& state) {
  nlohmann::json asyncResp;
  if (tryTakeAsyncResponse(state, "inspectMatchDetail", asyncResp)) {
    if (asyncResp.value("code", -1) == 0 && asyncResp.contains("match") && asyncResp["match"].is_object()) {
      state.historySelectedMatchDetail = asyncResp["match"];
    }
  }

  if (!state.showHistoryPopup || !state.historyPayload.is_object()) {
    ImGui::TextWrapped("点击房间页面中的玩家头像后，这里会展示对方信息（ELO、胜率、对战记录）。");
    if (ImGui::Button("Back")) {
      state.currentPage = state.inspectReturnPage;
    }
    return;
  }

  ImGui::Text("Target: %s", state.historyTargetName.c_str());

  if (state.historyPayload.contains("user") && state.historyPayload["user"].is_object()) {
    const auto& user = state.historyPayload["user"];
    const int wins = user.value("wins", 0);
    const int losses = user.value("losses", 0);
    const int total = wins + losses;
    const float rate = total > 0 ? (static_cast<float>(wins) * 100.0f / static_cast<float>(total)) : 0.0f;
    ImGui::Text("UserId: %lld", static_cast<long long>(user.value("userId", 0LL)));
    ImGui::Text("ELO: %d", user.value("elo", 1000));
    ImGui::Text("WinRate: %.1f%% (%d/%d)", rate, wins, total);
  }

  ImGui::SeparatorText("Match History");
  ImGui::BeginChild("HistoryList", ImVec2(0.0f, 200.0f), true);
  if (state.historyPayload.contains("history") && state.historyPayload["history"].is_array()) {
    for (const auto& h : state.historyPayload["history"]) {
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
  }
  ImGui::EndChild();

  if (state.historySelectedMatchDetail.is_object()) {
    ImGui::SeparatorText("Match Detail");
    ImGui::Text("MatchId: %s", state.historySelectedMatchDetail.value("matchId", std::string("-")).c_str());
    ImGui::Text("Winner: %s", state.historySelectedMatchDetail.value("winner", std::string("-")).c_str());
    ImGui::Text("Date: %s", formatEpoch(state.historySelectedMatchDetail.value("createdAtEpochSec", 0LL)).c_str());

    ImGui::Columns(2, "InspectMatchTeams", true);
    renderMatchTeams("Team Red", state.historySelectedMatchDetail.value("teamRed", nlohmann::json::array()));
    ImGui::NextColumn();
    renderMatchTeams("Team Blue", state.historySelectedMatchDetail.value("teamBlue", nlohmann::json::array()));
    ImGui::Columns(1);
  }

  if (ImGui::Button("Back")) {
    state.currentPage = state.inspectReturnPage;
  }
}

}  // namespace simpleelo::client::app::components
