#include "client/app/pages.h"

#include <algorithm>
#include <array>

#include <imgui.h>
#include <nlohmann/json.hpp>

#include "client/components/connectSections.h"
#include "client/uiCommon.h"
#include "protocol/api.h"

namespace simpleelo::client::app {

namespace {

template <size_t N>
std::string readBuffer(const std::array<char, N>& buffer) {
  const auto it = std::find(buffer.begin(), buffer.end(), '\0');
  return std::string(buffer.begin(), it);
}

}  // namespace

void renderConnectPage(simpleelo::client::ui::AppState& state, const RenderContext& ctx) {
  nlohmann::json resp;
  if (tryTakeAsyncResponse(state, "connectPing", resp)) {
    state.connectInProgress = false;
    state.lastResponse = resp.dump(2);
    if (resp.value("code", -1) == 0) {
      state.serverConnected = true;
      state.connectStatusIsError = false;
      state.connectStatusMessage = "连接成功";
      state.currentPage = static_cast<int>(ClientPage::Auth);
      simpleelo::client::ui::appendLog(state.logs, "ping -> " + resp.dump());
    } else {
      state.serverConnected = false;
      state.connectStatusIsError = true;
      state.connectStatusMessage = "连接失败: " + resp.value("message", std::string("未知错误"));
      simpleelo::client::ui::appendLog(state.logs, "ping failed -> " + resp.dump());
    }
  }

  beginStandardPageWindow("Connect Server", ctx);
  beginCenteredContentColumn("ConnectPageColumn", 980.0f);

  const float cardWidth = 560.0f;
  const float availWidth = ImGui::GetContentRegionAvail().x;
  const float offsetX = (availWidth - cardWidth) * 0.5f;
  if (offsetX > 0.0f) {
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
  }

  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.985f, 0.985f, 0.98f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.88f, 0.88f, 0.86f, 1.0f));
  ImGui::BeginChild("ConnectCenterCard", ImVec2(cardWidth, 0.0f), true);

  if (components::renderConnectActionSection(state)) {
    state.serverConnected = false;
    state.connectInProgress = true;
    state.connectStatusIsError = false;
    state.connectStatusMessage = "正在连接服务器...";
    const std::string host = readBuffer(state.host);
    const int port = state.port;
    if (host.empty()) {
      state.connectInProgress = false;
      state.connectStatusIsError = true;
      state.connectStatusMessage = "连接失败: Host 不能为空";
      simpleelo::client::ui::appendLog(state.logs, "connect failed -> empty host");
    } else if (port <= 0 || port > 65535) {
      state.connectInProgress = false;
      state.connectStatusIsError = true;
      state.connectStatusMessage = "连接失败: Port 必须在 1-65535";
      simpleelo::client::ui::appendLog(state.logs, "connect failed -> invalid port");
    } else {
      if (!startAsyncRequest(state, "connectPing", protocol::api::buildPingRequest(), 3.0)) {
        state.connectStatusIsError = false;
        state.connectStatusMessage = "已有连接请求进行中...";
      }
    }
  }

  components::renderConnectStatusSection(
      state.connectStatusMessage,
      state.connectStatusIsError,
      state.serverConnected,
      state.host.data(),
      state.port);

  ImGui::EndChild();
  ImGui::PopStyleColor(2);

  endCenteredContentColumn();
  ImGui::End();
}

}  // namespace simpleelo::client::app
