#include "client/components/connectSections.h"

#include <array>
#include <cstdio>

#include <imgui.h>

namespace simpleelo::client::app::components {

bool renderConnectActionSection(simpleelo::client::ui::AppState& state) {
  ImGui::SeparatorText("Connection Settings");
  ImGui::TextWrapped("配置服务器地址与端口，连接成功后会自动进入登录页。");

  ImGui::Dummy(ImVec2(0.0f, 4.0f));
  ImGui::TextUnformatted("Host");
  ImGui::SetNextItemWidth(-1.0f);
  ImGui::InputText("##connectHost", state.host.data(), state.host.size());

  ImGui::TextUnformatted("Port");
  ImGui::SetNextItemWidth(-1.0f);
  std::array<char, 16> portBuffer{};
  std::snprintf(portBuffer.data(), portBuffer.size(), "%d", state.port);
  if (ImGui::InputText("##connectPort", portBuffer.data(), portBuffer.size(), ImGuiInputTextFlags_CharsDecimal)) {
    int parsed = state.port;
    if (std::sscanf(portBuffer.data(), "%d", &parsed) == 1) {
      state.port = parsed;
    }
  }
  ImGui::Dummy(ImVec2(0.0f, 2.0f));

  const bool connectButtonDisabled = state.connectInProgress;
  if (connectButtonDisabled) {
    ImGui::BeginDisabled();
  }
  const bool clicked = ImGui::Button(state.connectInProgress ? "Connecting..." : "Connect", ImVec2(-1.0f, 0.0f));
  if (connectButtonDisabled) {
    ImGui::EndDisabled();
  }
  return clicked;
}

void renderConnectStatusSection(const std::string& message, bool isError, bool connected, const char* host, int port) {
  ImGui::SeparatorText("Status");
  if (!message.empty()) {
    const ImVec4 color = isError ? ImVec4(0.62f, 0.25f, 0.24f, 1.0f) : ImVec4(0.24f, 0.44f, 0.26f, 1.0f);
    ImGui::TextColored(color, "%s", message.c_str());
  }

  ImGui::SeparatorText("Current");
  ImGui::Text("%s:%d", host, port);
  ImGui::Text("Status: %s", connected ? "connected" : "not connected");
}

}  // namespace simpleelo::client::app::components
