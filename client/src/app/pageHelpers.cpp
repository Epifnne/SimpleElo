#include "client/app/pageHelpers.h"

#include <chrono>
#include <exception>

#include <imgui.h>

#include "client/app/pages.h"
#include "client/net/httpClient.h"
#include "client/uiCommon.h"
#include "protocol/api.h"

namespace simpleelo::client::app {

namespace {

bool isTransportFailure(const nlohmann::json& resp) {
  return resp.value("code", 0) == -1;
}

void applyDisconnectedState(simpleelo::client::ui::AppState& state, const std::string& reason) {
  state.serverConnected = false;
  state.loggedIn = false;
  state.token.clear();
  state.selectedRoomId[0] = '\0';
  state.roomDetail = nlohmann::json{};
  state.asyncFutureByKey.clear();
  state.asyncDeadlineByKey.clear();
  state.asyncResponseByKey.clear();
  state.currentPage = static_cast<int>(ClientPage::Connect);
  state.connectStatusIsError = true;
  state.connectStatusMessage = "连接断开: " + reason;
}

}  // namespace

void beginStandardPageWindow(const char* title, const RenderContext& ctx, int extraWindowFlags) {
  const float outerMargin = ctx.width >= 1360 ? 30.0f : 22.0f;
  const float topOffset = 62.0f;
  const float bottomMargin = 20.0f;

  ImGui::SetNextWindowPos(ImVec2(outerMargin, topOffset), ImGuiCond_Always);
  ImGui::SetNextWindowSize(
      ImVec2(static_cast<float>(ctx.width) - outerMargin * 2.0f,
             static_cast<float>(ctx.height) - topOffset - bottomMargin),
      ImGuiCond_Always);
  ImGui::Begin(title,
               nullptr,
               ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
                   ImGuiWindowFlags_NoScrollWithMouse | static_cast<ImGuiWindowFlags>(extraWindowFlags));
}

void beginCenteredContentColumn(const char* id, float maxWidth) {
  const float availWidth = ImGui::GetContentRegionAvail().x;
  const float contentWidth = availWidth > maxWidth ? maxWidth : availWidth;
  const float offsetX = (availWidth - contentWidth) * 0.5f;
  if (offsetX > 0.0f) {
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
  }

  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.0f, 10.0f));
  ImGui::BeginChild(id, ImVec2(contentWidth, 0.0f), false, ImGuiWindowFlags_None);
}

void endCenteredContentColumn() {
  ImGui::EndChild();
  ImGui::PopStyleVar();
}

nlohmann::json sendRequest(simpleelo::client::ui::AppState& state, const nlohmann::json& request) {
  try {
    const std::string response = simpleelo::client::net::sendJsonLine(state.host.data(), state.port, request.dump());
    return nlohmann::json::parse(response);
  } catch (const std::exception& ex) {
    return nlohmann::json{{"code", -1}, {"message", ex.what()}};
  }
}

bool startAsyncRequest(simpleelo::client::ui::AppState& state,
                       const std::string& key,
                       const nlohmann::json& request,
                       double timeoutSeconds) {
  if (state.asyncFutureByKey.find(key) != state.asyncFutureByKey.end()) {
    return false;
  }

  const std::string host = state.host.data();
  const int port = state.port;
  const std::string payload = request.dump();
  auto fut = std::async(std::launch::async, [host, port, payload]() {
    try {
      const std::string response = simpleelo::client::net::sendJsonLine(host, port, payload);
      return nlohmann::json::parse(response);
    } catch (const std::exception& ex) {
      return nlohmann::json{{"code", -1}, {"message", ex.what()}};
    }
  });

  state.asyncFutureByKey[key] = fut.share();
  state.asyncDeadlineByKey[key] = std::chrono::steady_clock::now() + std::chrono::milliseconds(static_cast<int>(timeoutSeconds * 1000.0));
  return true;
}

bool isAsyncRequestPending(const simpleelo::client::ui::AppState& state, const std::string& key) {
  return state.asyncFutureByKey.find(key) != state.asyncFutureByKey.end();
}

bool tryTakeAsyncResponse(simpleelo::client::ui::AppState& state, const std::string& key, nlohmann::json& response) {
  const auto it = state.asyncResponseByKey.find(key);
  if (it == state.asyncResponseByKey.end()) {
    return false;
  }
  response = it->second;
  state.asyncResponseByKey.erase(it);
  return true;
}

void pumpAsyncRequests(simpleelo::client::ui::AppState& state) {
  std::vector<std::string> readyKeys;
  std::vector<std::string> timeoutKeys;
  const auto now = std::chrono::steady_clock::now();

  for (const auto& [key, fut] : state.asyncFutureByKey) {
    if (fut.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
      readyKeys.push_back(key);
      continue;
    }

    const auto deadlineIt = state.asyncDeadlineByKey.find(key);
    if (deadlineIt != state.asyncDeadlineByKey.end() && now > deadlineIt->second) {
      timeoutKeys.push_back(key);
    }
  }

  for (const auto& key : readyKeys) {
    auto futIt = state.asyncFutureByKey.find(key);
    if (futIt == state.asyncFutureByKey.end()) {
      continue;
    }
    const nlohmann::json resp = futIt->second.get();
    state.asyncFutureByKey.erase(futIt);
    state.asyncDeadlineByKey.erase(key);
    state.asyncResponseByKey[key] = resp;
    state.lastResponse = resp.dump(2);

    if (isTransportFailure(resp)) {
      applyDisconnectedState(state, resp.value("message", std::string("network error")));
    }
  }

  for (const auto& key : timeoutKeys) {
    state.asyncFutureByKey.erase(key);
    state.asyncDeadlineByKey.erase(key);
    state.networkTimeoutPopupOpen = true;
    state.networkTimeoutPopupMessage = "网络连接超时: " + key;
    applyDisconnectedState(state, "请求超时");
  }

  nlohmann::json resp;
  if (tryTakeAsyncResponse(state, "refreshProfile", resp)) {
    if (resp.value("code", -1) == 0 && resp.contains("user")) {
      updateSelfFromUser(state, resp["user"]);
    }
  }
  if (tryTakeAsyncResponse(state, "refreshRooms", resp)) {
    if (resp.value("code", -1) == 0 && resp.contains("rooms") && resp["rooms"].is_array()) {
      state.roomList.clear();
      for (const auto& r : resp["rooms"]) {
        state.roomList.push_back(r);
      }
    }
  }
  if (tryTakeAsyncResponse(state, "refreshRoomDetail", resp)) {
    if (resp.value("code", -1) == 0) {
      state.roomDetail = resp;
    } else {
      const int code = resp.value("code", -1);
      if (code == 438 || code == 437) {
        state.globalNotice = "room has been deleted";
        state.globalNoticeUntil = ImGui::GetTime() + 4.0;
        state.selectedRoomId[0] = '\0';
        state.roomDetail = nlohmann::json{};
        state.currentPage = static_cast<int>(ClientPage::Lobby);
      }
    }
  }
}

bool handleValidationFailure(simpleelo::client::ui::AppState& state,
                             const protocol::ApiResponse& validation,
                             const std::string& operation) {
  if (validation.code == 0) {
    return false;
  }
  nlohmann::json err = {{"code", validation.code}, {"message", validation.message}};
  state.lastResponse = err.dump(2);
  simpleelo::client::ui::appendLog(state.logs, operation + "(local-validate) -> " + err.dump());
  return true;
}

void updateSelfFromUser(simpleelo::client::ui::AppState& state, const nlohmann::json& user) {
  state.selfUserId = user.value("userId", 0LL);
  state.selfNickname = user.value("nickname", user.value("email", "player"));
  state.selfElo = user.value("elo", 1000);
  simpleelo::client::ui::copyToBuffer(state.profileNicknameDraft.data(), state.profileNicknameDraft.size(), state.selfNickname);
}

void refreshProfile(simpleelo::client::ui::AppState& state) {
  if (state.token.empty()) {
    return;
  }
  (void)startAsyncRequest(state, "refreshProfile", protocol::api::buildGetProfileRequest(state.token));
}

void refreshRooms(simpleelo::client::ui::AppState& state) {
  if (state.token.empty()) {
    return;
  }
  (void)startAsyncRequest(state, "refreshRooms", protocol::api::buildListRoomsRequest(state.token));
}

void refreshRoomDetail(simpleelo::client::ui::AppState& state) {
  if (state.token.empty() || std::string(state.selectedRoomId.data()).empty()) {
    return;
  }
  (void)startAsyncRequest(state,
                          "refreshRoomDetail",
                          protocol::api::buildGetRoomDetailRequest(state.token, state.selectedRoomId.data()));
}

}  // namespace simpleelo::client::app
