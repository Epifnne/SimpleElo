#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "client/ui/appState.h"
#include "protocol/messages.h"

namespace simpleelo::client::app {

struct RenderContext {
  int width = 1080;
  int height = 720;
};

void beginStandardPageWindow(const char* title, const RenderContext& ctx, int extraWindowFlags = 0);
void beginCenteredContentColumn(const char* id, float maxWidth = 980.0f);
void endCenteredContentColumn();

nlohmann::json sendRequest(simpleelo::client::ui::AppState& state, const nlohmann::json& request);
bool startAsyncRequest(simpleelo::client::ui::AppState& state,
                       const std::string& key,
                       const nlohmann::json& request,
                       double timeoutSeconds = 3.0);
bool isAsyncRequestPending(const simpleelo::client::ui::AppState& state, const std::string& key);
bool tryTakeAsyncResponse(simpleelo::client::ui::AppState& state, const std::string& key, nlohmann::json& response);
void pumpAsyncRequests(simpleelo::client::ui::AppState& state);
bool handleValidationFailure(simpleelo::client::ui::AppState& state,
                             const protocol::ApiResponse& validation,
                             const std::string& operation);
void updateSelfFromUser(simpleelo::client::ui::AppState& state, const nlohmann::json& user);
void refreshProfile(simpleelo::client::ui::AppState& state);
void refreshRooms(simpleelo::client::ui::AppState& state);
void refreshRoomDetail(simpleelo::client::ui::AppState& state);

}  // namespace simpleelo::client::app
