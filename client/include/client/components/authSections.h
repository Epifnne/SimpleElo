#pragma once

#include "client/ui/appState.h"

namespace simpleelo::client::app::components {

struct AuthRenderActions {
  bool switchServer = false;
  bool submitLogin = false;
  bool sendCode = false;
  bool submitRegister = false;
  bool resetPassword = false;
};

AuthRenderActions renderAuthSections(simpleelo::client::ui::AppState& state,
                                     int cooldownRemain,
                                     bool loginBusy,
                                     bool registerBusy,
                                     bool sendBusy,
                                     bool canSendCode);

}  // namespace simpleelo::client::app::components
