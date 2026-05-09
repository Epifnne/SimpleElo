#include "client/components/roomResultSection.h"

#include <string>

#include <imgui.h>

#include "client/app/pages.h"
#include "client/uiCommon.h"
#include "protocol/api.h"

namespace simpleelo::client::app::components {

void renderRoomResultSection(simpleelo::client::ui::AppState& state, bool isOwner) {
  nlohmann::json asyncResp;
  if (tryTakeAsyncResponse(state, "roomSubmitMatch", asyncResp)) {
    if (asyncResp.value("code", -1) == 0 && asyncResp.contains("match") && asyncResp["match"].is_object()) {
      simpleelo::client::ui::copyToBuffer(state.roomMatchId.data(), state.roomMatchId.size(),
                                          asyncResp["match"].value("matchId", std::string{}));
    }
  }
  (void)tryTakeAsyncResponse(state, "roomVoteMatch", asyncResp);
  (void)tryTakeAsyncResponse(state, "roomGetMatch", asyncResp);

  ImGui::SeparatorText("Match Report / Vote");
  const char* winners[] = {"red", "blue"};
  ImGui::Combo("Winner", &state.roomWinnerIndex, winners, IM_ARRAYSIZE(winners));
  if (isOwner && ImGui::Button("Submit Match Result")) {
    const std::string rid = state.selectedRoomId.data();
    const std::string matchId = rid + "-" + std::to_string(static_cast<long long>(ImGui::GetTime()));
    const std::string idempotency = rid + "-" + winners[state.roomWinnerIndex] + "-" + std::to_string(static_cast<long long>(ImGui::GetTime()));
    (void)startAsyncRequest(state,
                            "roomSubmitMatch",
                            {{"action", protocol::api::kActionSubmitMatch},
                             {"token", state.token},
                             {"roomId", rid},
                             {"winner", winners[state.roomWinnerIndex]},
                             {"matchId", matchId},
                             {"idempotencyKey", idempotency}});
  }

  ImGui::InputText("MatchId", state.roomMatchId.data(), state.roomMatchId.size());
  ImGui::Checkbox("Vote Approve", &state.roomVoteApprove);
  if (ImGui::Button("Vote Current Match")) {
    (void)startAsyncRequest(state,
                            "roomVoteMatch",
                            {{"action", protocol::api::kActionVote},
                             {"token", state.token},
                             {"matchId", state.roomMatchId.data()},
                             {"approve", state.roomVoteApprove}});
  }
  ImGui::SameLine();
  if (ImGui::Button("Refresh Match Status")) {
    (void)startAsyncRequest(state,
                            "roomGetMatch",
                            protocol::api::buildGetMatchRequest(state.token, state.roomMatchId.data()));
  }

}

}  // namespace simpleelo::client::app::components
