#include "client/components/authSections.h"

#include <cfloat>
#include <string>

#include <imgui.h>

namespace simpleelo::client::app::components {
namespace {

struct AuthUiConstants {
  static constexpr float kSpacing4 = 4.0f;
  static constexpr float kSpacing8 = 8.0f;
  static constexpr float kSpacing12 = 12.0f;

  static constexpr float kCardWidth = 420.0f;
  static constexpr float kInputHeight = 34.0f;
  static constexpr float kButtonHeight = 36.0f;
  static constexpr float kRadius = 6.0f;
  static constexpr float kSendCodeButtonWidth = 120.0f;
};

void drawFieldError(const std::string& text) {
  if (text.empty()) {
    return;
  }
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.63f, 0.24f, 0.23f, 1.0f));
  ImGui::TextWrapped("%s", text.c_str());
  ImGui::PopStyleColor();
}

void drawInlineSuccess(const std::string& text) {
  if (text.empty()) {
    return;
  }
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.23f, 0.40f, 0.24f, 1.0f));
  ImGui::TextWrapped("%s", text.c_str());
  ImGui::PopStyleColor();
}

void drawInlineError(const std::string& text) {
  if (text.empty()) {
    return;
  }
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.63f, 0.24f, 0.23f, 1.0f));
  ImGui::TextWrapped("%s", text.c_str());
  ImGui::PopStyleColor();
}

void pushSecondaryWeakButtonStyle() {
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.92f, 0.92f, 0.90f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.88f, 0.88f, 0.86f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.84f, 0.84f, 0.82f, 1.0f));
}

void popSecondaryWeakButtonStyle() {
  ImGui::PopStyleColor(3);
}

}  // namespace

AuthRenderActions renderAuthSections(simpleelo::client::ui::AppState& state,
                                     int cooldownRemain,
                                     bool loginBusy,
                                     bool registerBusy,
                                     bool sendBusy,
                                     bool canSendCode) {
  AuthRenderActions actions;

  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, AuthUiConstants::kRadius);
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(AuthUiConstants::kSpacing8, AuthUiConstants::kSpacing8));
  const float inputPadY = (AuthUiConstants::kInputHeight - ImGui::GetFontSize()) * 0.5f;
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(AuthUiConstants::kSpacing8, inputPadY));

  const float contentWidth = ImGui::GetContentRegionAvail().x;
  const float offsetX = (contentWidth - AuthUiConstants::kCardWidth) > 0.0f
                          ? (contentWidth - AuthUiConstants::kCardWidth) * 0.5f
                          : 0.0f;
  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);

  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.985f, 0.985f, 0.98f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.88f, 0.88f, 0.86f, 1.0f));
  ImGui::BeginChild("AuthCard", ImVec2(AuthUiConstants::kCardWidth, 0.0f), true);
  ImGui::Text("Server: %s:%d", state.host.data(), state.port);
  ImGui::SameLine();
  pushSecondaryWeakButtonStyle();
  actions.switchServer = ImGui::Button("Switch Server", ImVec2(110.0f, AuthUiConstants::kButtonHeight));
  popSecondaryWeakButtonStyle();

  ImGui::Dummy(ImVec2(0.0f, AuthUiConstants::kSpacing12));

  if (ImGui::BeginTabBar("AuthTabs")) {
    if (ImGui::BeginTabItem("Login")) {
      state.authActiveTab = 0;

      ImGui::TextUnformatted("Email");
      ImGui::SetNextItemWidth(-FLT_MIN);
      if (loginBusy) {
        ImGui::BeginDisabled();
      }
      ImGui::InputText("##loginEmail", state.email.data(), state.email.size());
      if (loginBusy) {
        ImGui::EndDisabled();
      }
      drawFieldError(state.authLoginEmailError);

      ImGui::TextUnformatted("Password");
      ImGui::SetNextItemWidth(-FLT_MIN);
      if (loginBusy) {
        ImGui::BeginDisabled();
      }
      ImGui::InputText("##loginPassword", state.password.data(), state.password.size(), ImGuiInputTextFlags_Password);
      if (loginBusy) {
        ImGui::EndDisabled();
      }
      drawFieldError(state.authLoginPasswordError);

      drawInlineSuccess(state.authLoginInlineSuccess);
      drawInlineError(state.authLoginInlineError);

      ImGui::Dummy(ImVec2(0.0f, AuthUiConstants::kSpacing4));
      const char* loginBtnText = state.authLoginSubmitting ? "Signing in..." : "Login";
      if (loginBusy) {
        ImGui::BeginDisabled();
      }
      actions.submitLogin = ImGui::Button(loginBtnText, ImVec2(-FLT_MIN, AuthUiConstants::kButtonHeight));
      if (loginBusy) {
        ImGui::EndDisabled();
      }

      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Register")) {
      state.authActiveTab = 1;

      ImGui::TextUnformatted("Email");
      ImGui::SetNextItemWidth(-FLT_MIN);
      if (registerBusy) {
        ImGui::BeginDisabled();
      }
      ImGui::InputText("##registerEmail", state.email.data(), state.email.size());
      if (registerBusy) {
        ImGui::EndDisabled();
      }
      drawFieldError(state.authRegisterEmailError);

      ImGui::TextUnformatted("Verification Code");
      const float rowWidth = ImGui::GetContentRegionAvail().x;
      const float codeInputWidth = rowWidth - AuthUiConstants::kSendCodeButtonWidth - AuthUiConstants::kSpacing8;
      if (registerBusy) {
        ImGui::BeginDisabled();
      }
      ImGui::SetNextItemWidth(codeInputWidth);
      ImGui::InputText("##registerCode", state.verifyCode.data(), state.verifyCode.size());
      ImGui::SameLine(0.0f, AuthUiConstants::kSpacing8);

      std::string sendCodeText;
      if (sendBusy) {
        sendCodeText = "Sending...";
      } else if (cooldownRemain > 0) {
        sendCodeText = "Retry (" + std::to_string(cooldownRemain) + "s)";
      } else {
        sendCodeText = "Send Code";
      }

      pushSecondaryWeakButtonStyle();
      if (!canSendCode) {
        ImGui::BeginDisabled();
      }
      actions.sendCode = ImGui::Button(sendCodeText.c_str(), ImVec2(AuthUiConstants::kSendCodeButtonWidth, AuthUiConstants::kButtonHeight));
      if (!canSendCode) {
        ImGui::EndDisabled();
      }
      popSecondaryWeakButtonStyle();
      if (registerBusy) {
        ImGui::EndDisabled();
      }
      drawFieldError(state.authRegisterCodeError);

      ImGui::TextUnformatted("Password");
      ImGui::SetNextItemWidth(-FLT_MIN);
      if (registerBusy) {
        ImGui::BeginDisabled();
      }
      ImGui::InputText("##registerPassword", state.password.data(), state.password.size(), ImGuiInputTextFlags_Password);
      if (registerBusy) {
        ImGui::EndDisabled();
      }
      drawFieldError(state.authRegisterPasswordError);

      ImGui::TextUnformatted("Confirm Password");
      ImGui::SetNextItemWidth(-FLT_MIN);
      if (registerBusy) {
        ImGui::BeginDisabled();
      }
      ImGui::InputText("##registerConfirmPassword", state.confirmPassword.data(), state.confirmPassword.size(), ImGuiInputTextFlags_Password);
      if (registerBusy) {
        ImGui::EndDisabled();
      }
      drawFieldError(state.authRegisterConfirmPasswordError);

      drawInlineSuccess(state.authRegisterInlineSuccess);
      drawInlineError(state.authRegisterInlineError);

      ImGui::Dummy(ImVec2(0.0f, AuthUiConstants::kSpacing4));
      const char* registerBtnText = state.authRegisterSubmitting ? "Creating..." : "Register";
      if (registerBusy) {
        ImGui::BeginDisabled();
      }
      actions.submitRegister = ImGui::Button(registerBtnText, ImVec2(-FLT_MIN, AuthUiConstants::kButtonHeight));
      if (registerBusy) {
        ImGui::EndDisabled();
      }

      ImGui::Dummy(ImVec2(0.0f, AuthUiConstants::kSpacing4));
      const char* resetBtnText = state.authRegisterSubmitting ? "Resetting..." : "Reset Password";
      if (registerBusy) {
        ImGui::BeginDisabled();
      }
      actions.resetPassword = ImGui::Button(resetBtnText, ImVec2(-FLT_MIN, AuthUiConstants::kButtonHeight));
      if (registerBusy) {
        ImGui::EndDisabled();
      }

      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }

  ImGui::EndChild();
  ImGui::PopStyleColor(2);
  ImGui::PopStyleVar(3);

  return actions;
}

}  // namespace simpleelo::client::app::components
