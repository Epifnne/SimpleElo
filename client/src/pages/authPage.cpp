#include "client/app/pages.h"

#include <cmath>
#include <cstring>
#include <cfloat>
#include <string>

#include <imgui.h>

#include "client/components/authSections.h"
#include "client/uiCommon.h"
#include "protocol/api.h"

namespace simpleelo::client::app {

namespace {

struct AuthUiConstants {
  static constexpr double kCooldownSeconds = 60.0;
  static constexpr double kLoadingSeconds = 0.6;
};

enum class PendingAction {
  None = 0,
  SendCode = 1,
  Login = 2,
  Register = 3,
  ResetPassword = 4,
};

bool isEmailValid(const char* email) {
  if (email == nullptr) {
    return false;
  }
  const std::string value(email);
  const auto atPos = value.find('@');
  const auto dotPos = value.rfind('.');
  if (value.size() < 5) {
    return false;
  }
  if (atPos == std::string::npos || dotPos == std::string::npos) {
    return false;
  }
  if (atPos == 0 || dotPos <= atPos + 1 || dotPos >= value.size() - 1) {
    return false;
  }
  return true;
}

bool isPasswordValid(const char* password) {
  return password != nullptr && std::strlen(password) >= 6;
}

bool validateLoginForm(simpleelo::client::ui::AppState& state) {
  state.authLoginEmailError.clear();
  state.authLoginPasswordError.clear();
  state.authLoginInlineError.clear();
  state.authLoginInlineSuccess.clear();

  bool ok = true;
  if (!isEmailValid(state.email.data())) {
    state.authLoginEmailError = "Please enter a valid email.";
    ok = false;
  }
  if (!isPasswordValid(state.password.data())) {
    state.authLoginPasswordError = "Password must be at least 6 characters.";
    ok = false;
  }
  return ok;
}

bool validateRegisterForm(simpleelo::client::ui::AppState& state) {
  state.authRegisterEmailError.clear();
  state.authRegisterCodeError.clear();
  state.authRegisterPasswordError.clear();
  state.authRegisterConfirmPasswordError.clear();
  state.authRegisterInlineError.clear();
  state.authRegisterInlineSuccess.clear();

  bool ok = true;
  if (!isEmailValid(state.email.data())) {
    state.authRegisterEmailError = "Please enter a valid email.";
    ok = false;
  }
  if (std::strlen(state.verifyCode.data()) == 0) {
    state.authRegisterCodeError = "Verification code is required.";
    ok = false;
  }
  if (!isPasswordValid(state.password.data())) {
    state.authRegisterPasswordError = "Password must be at least 6 characters.";
    ok = false;
  }
  if (std::strcmp(state.password.data(), state.confirmPassword.data()) != 0) {
    state.authRegisterConfirmPasswordError = "Passwords do not match.";
    ok = false;
  }
  return ok;
}

void startSendCode(simpleelo::client::ui::AppState& state, double now) {
  state.authSendCodeLoading = true;
  state.authSendCodeLoadingUntil = now + AuthUiConstants::kLoadingSeconds;
  state.authPendingAction = static_cast<int>(PendingAction::SendCode);
  state.authRegisterInlineError.clear();
  state.authRegisterInlineSuccess.clear();
  state.authRegisterCodeError.clear();
}

void startLoginSubmit(simpleelo::client::ui::AppState& state, double now) {
  state.authLoginSubmitting = true;
  state.authLoginSubmittingUntil = now + AuthUiConstants::kLoadingSeconds;
  state.authPendingAction = static_cast<int>(PendingAction::Login);
}

void startRegisterSubmit(simpleelo::client::ui::AppState& state, double now) {
  state.authRegisterSubmitting = true;
  state.authRegisterSubmittingUntil = now + AuthUiConstants::kLoadingSeconds;
  state.authPendingAction = static_cast<int>(PendingAction::Register);
}

void startResetPassword(simpleelo::client::ui::AppState& state, double now) {
  state.authRegisterSubmitting = true;
  state.authRegisterSubmittingUntil = now + AuthUiConstants::kLoadingSeconds;
  state.authPendingAction = static_cast<int>(PendingAction::ResetPassword);
}

void processPendingActions(simpleelo::client::ui::AppState& state) {
  const double now = ImGui::GetTime();
  const auto pending = static_cast<PendingAction>(state.authPendingAction);
  if (pending == PendingAction::None) {
    return;
  }

  if (pending == PendingAction::SendCode) {
    if (!state.authSendCodeLoading || now < state.authSendCodeLoadingUntil) {
      return;
    }

    const protocol::api::SendCodeRequest request{state.email.data()};
    const auto validation = protocol::api::validateSendCodeRequest(request);
    if (validation.code != 0) {
      state.authRegisterCodeError = validation.message;
      state.authPendingAction = static_cast<int>(PendingAction::None);
      return;
    }

    nlohmann::json resp;
    if (!tryTakeAsyncResponse(state, "authSendCode", resp)) {
      (void)startAsyncRequest(state, "authSendCode", protocol::api::buildSendCodeRequest(request));
      return;
    }
    state.authSendCodeLoading = false;
    simpleelo::client::ui::appendLog(state.logs, "sendCode(async) -> " + resp.dump());
    if (resp.value("code", -1) == 0) {
      state.authRegisterInlineSuccess = "Verification code sent.";
      state.authRegisterInlineError.clear();
      state.authSendCodeCooldownUntil = now + AuthUiConstants::kCooldownSeconds;
    } else {
      state.authRegisterInlineError = resp.value("message", "Failed to send verification code.");
      state.authRegisterInlineSuccess.clear();
    }
    state.authPendingAction = static_cast<int>(PendingAction::None);
    return;
  }

  if (pending == PendingAction::Login) {
    if (!state.authLoginSubmitting || now < state.authLoginSubmittingUntil) {
      return;
    }

    const protocol::LoginRequest request{state.email.data(), state.password.data()};
    const auto validation = protocol::api::validateLoginRequest(request);
    if (validation.code != 0) {
      state.authLoginInlineError = validation.message;
      state.authPendingAction = static_cast<int>(PendingAction::None);
      return;
    }

    nlohmann::json resp;
    if (!tryTakeAsyncResponse(state, "authLogin", resp)) {
      (void)startAsyncRequest(state, "authLogin", protocol::api::buildLoginRequest(request));
      return;
    }
    state.authLoginSubmitting = false;
    simpleelo::client::ui::appendLog(state.logs, "login(async) -> " + resp.dump());

    if (resp.value("code", -1) == 0) {
      state.authLoginInlineSuccess = "Login successful.";
      state.authLoginInlineError.clear();
      state.loggedIn = true;
      state.token = resp.value("token", "");
      if (resp.contains("user")) {
        updateSelfFromUser(state, resp["user"]);
      }
      refreshRooms(state);
      state.currentPage = static_cast<int>(ClientPage::Lobby);
    } else {
      state.authLoginInlineError = resp.value("message", "Login failed.");
      state.authLoginInlineSuccess.clear();
    }
    state.authPendingAction = static_cast<int>(PendingAction::None);
    return;
  }

  if (pending == PendingAction::Register) {
    if (!state.authRegisterSubmitting || now < state.authRegisterSubmittingUntil) {
      return;
    }

    const protocol::RegisterRequest request{state.email.data(), state.password.data(), state.verifyCode.data()};
    const auto validation = protocol::api::validateRegisterRequest(request);
    if (validation.code != 0) {
      state.authRegisterInlineError = validation.message;
      state.authPendingAction = static_cast<int>(PendingAction::None);
      return;
    }

    nlohmann::json resp;
    if (!tryTakeAsyncResponse(state, "authRegister", resp)) {
      (void)startAsyncRequest(state, "authRegister", protocol::api::buildRegisterRequest(request, state.nickname.data()));
      return;
    }
    state.authRegisterSubmitting = false;
    simpleelo::client::ui::appendLog(state.logs, "register(async) -> " + resp.dump());

    if (resp.value("code", -1) == 0) {
      state.authRegisterInlineSuccess = "Registration successful. Please log in.";
      state.authRegisterInlineError.clear();
      state.authActiveTab = 0;
    } else {
      state.authRegisterInlineError = resp.value("message", "Registration failed.");
      state.authRegisterInlineSuccess.clear();
    }
    state.authPendingAction = static_cast<int>(PendingAction::None);
    return;
  }

  if (pending == PendingAction::ResetPassword) {
    if (!state.authRegisterSubmitting || now < state.authRegisterSubmittingUntil) {
      return;
    }

    if (!isEmailValid(state.email.data())) {
      state.authRegisterEmailError = "Please enter a valid email.";
      state.authPendingAction = static_cast<int>(PendingAction::None);
      return;
    }
    if (std::strlen(state.verifyCode.data()) == 0) {
      state.authRegisterCodeError = "Verification code is required.";
      state.authPendingAction = static_cast<int>(PendingAction::None);
      return;
    }
    if (!isPasswordValid(state.password.data())) {
      state.authRegisterPasswordError = "Password must be at least 6 characters.";
      state.authPendingAction = static_cast<int>(PendingAction::None);
      return;
    }
    if (std::strcmp(state.password.data(), state.confirmPassword.data()) != 0) {
      state.authRegisterConfirmPasswordError = "Passwords do not match.";
      state.authPendingAction = static_cast<int>(PendingAction::None);
      return;
    }

    const protocol::api::ResetPasswordRequest request{
        state.email.data(),
        state.verifyCode.data(),
        state.password.data()};
    const auto validation = protocol::api::validateResetPasswordRequest(request);
    if (validation.code != 0) {
      state.authRegisterInlineError = validation.message;
      state.authPendingAction = static_cast<int>(PendingAction::None);
      return;
    }

    nlohmann::json resp;
    if (!tryTakeAsyncResponse(state, "authResetPassword", resp)) {
      (void)startAsyncRequest(state, "authResetPassword", protocol::api::buildResetPasswordRequest(request));
      return;
    }
    state.authRegisterSubmitting = false;
    simpleelo::client::ui::appendLog(state.logs, "resetPassword(async) -> " + resp.dump());
    if (resp.value("code", -1) == 0) {
      state.authRegisterInlineSuccess = "Password reset successful. Please log in.";
      state.authRegisterInlineError.clear();
      state.authActiveTab = 0;
    } else {
      state.authRegisterInlineError = resp.value("message", "Reset password failed.");
      state.authRegisterInlineSuccess.clear();
    }
    state.authPendingAction = static_cast<int>(PendingAction::None);
  }
}

}  // namespace

void renderAuthPage(simpleelo::client::ui::AppState& state, const RenderContext& ctx) {
  processPendingActions(state);

  beginStandardPageWindow("Login / Register", ctx);
  beginCenteredContentColumn("AuthPageColumn", 960.0f);

  const double now = ImGui::GetTime();
  int cooldownRemain = 0;
  if (state.authSendCodeCooldownUntil > now) {
    cooldownRemain = static_cast<int>(std::ceil(state.authSendCodeCooldownUntil - now));
  } else {
    state.authSendCodeCooldownUntil = 0.0;
  }

  const bool noPendingAction = state.authPendingAction == static_cast<int>(PendingAction::None);
  const bool loginBusy = state.authLoginSubmitting || !noPendingAction;
  const bool sendBusy = state.authSendCodeLoading;
  const bool submitBusy = state.authRegisterSubmitting;
  const bool registerBusy = sendBusy || submitBusy || !noPendingAction;
  const bool canSendCode = isEmailValid(state.email.data()) && cooldownRemain == 0 && !sendBusy && noPendingAction;

  const auto actions = components::renderAuthSections(
      state,
      cooldownRemain,
      loginBusy,
      registerBusy,
      sendBusy,
      canSendCode);

  if (actions.switchServer) {
    state.serverConnected = false;
    state.currentPage = static_cast<int>(ClientPage::Connect);
    endCenteredContentColumn();
    ImGui::End();
    return;
  }

  if (actions.sendCode && canSendCode) {
    startSendCode(state, now);
  }

  if (actions.submitLogin && !state.authLoginSubmitting && noPendingAction) {
    if (validateLoginForm(state)) {
      startLoginSubmit(state, now);
    }
  }

  if (actions.submitRegister && !state.authRegisterSubmitting && noPendingAction) {
    if (validateRegisterForm(state)) {
      startRegisterSubmit(state, now);
    }
  }

  if (actions.resetPassword && !state.authRegisterSubmitting && noPendingAction) {
    state.authRegisterEmailError.clear();
    state.authRegisterCodeError.clear();
    state.authRegisterPasswordError.clear();
    state.authRegisterConfirmPasswordError.clear();
    state.authRegisterInlineError.clear();
    state.authRegisterInlineSuccess.clear();
    startResetPassword(state, now);
  }

  endCenteredContentColumn();
  ImGui::End();
}

}  // namespace simpleelo::client::app
