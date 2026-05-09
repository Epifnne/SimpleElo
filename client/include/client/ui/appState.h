#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <future>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

namespace simpleelo::client::ui {

struct AppState {
  int currentPage = 0;
  bool serverConnected = false;
  int inspectReturnPage = 3;

  std::array<char, 64> host{"127.0.0.1"};
  int port = 18080;
  bool connectInProgress = false;
  bool connectStatusIsError = false;
  std::string connectStatusMessage;
  std::future<nlohmann::json> connectFuture;

  std::array<char, 128> email{"player@example.com"};
  std::array<char, 64> password{"Pass!123"};
  std::array<char, 16> verifyCode{"123456"};
  std::array<char, 64> confirmPassword{};
  std::array<char, 64> nickname{"player"};

  int authActiveTab = 0;

  std::string authLoginEmailError;
  std::string authLoginPasswordError;
  std::string authLoginInlineError;
  std::string authLoginInlineSuccess;

  std::string authRegisterEmailError;
  std::string authRegisterCodeError;
  std::string authRegisterPasswordError;
  std::string authRegisterConfirmPasswordError;
  std::string authRegisterInlineError;
  std::string authRegisterInlineSuccess;

  bool authSendCodeLoading = false;
  double authSendCodeLoadingUntil = 0.0;
  double authSendCodeCooldownUntil = 0.0;

  bool authLoginSubmitting = false;
  double authLoginSubmittingUntil = 0.0;

  bool authRegisterSubmitting = false;
  double authRegisterSubmittingUntil = 0.0;

  int authPendingAction = 0;

  bool loggedIn = false;
  std::string token;
  std::int64_t selfUserId = 0;
  std::string selfNickname;
  int selfElo = 1000;

  std::array<char, 64> profileNicknameDraft{"player"};
  std::array<char, 128> profileAvatarDraft{};
  std::array<char, 64> profilePasswordDraft{};
  std::array<char, 64> profilePasswordConfirmDraft{};

  std::array<char, 64> createRoomName{"RankedRoom"};
  std::array<char, 64> createRoomPassword{};
  bool createRoomPopup = false;

  std::array<char, 64> selectedRoomId{};
  std::array<char, 64> joinRoomPassword{};
  std::array<char, 64> lobbyRoomKeyword{};
  bool lobbyOnlyJoinable = false;

  int selfTeamIndex = 0;
  std::array<char, 64> selfRole{"captain"};
  int selfRoleScore = 0;
  bool selfReady = false;
  bool selfLeftEarly = false;

  int roomDraftTeamIndex = 0;
  std::array<char, 64> roomDraftRole{"captain"};
  int roomDraftRoleScore = 0;
  bool roomDraftReady = false;
  bool roomSlotEditPending = false;

  int roomWinnerIndex = 0;
  std::array<char, 64> roomMatchId{};
  bool roomVoteApprove = true;

  std::vector<nlohmann::json> roomList;
  nlohmann::json roomDetail;

  std::string globalNotice;
  double globalNoticeUntil = 0.0;

  bool showHistoryPopup = false;
  std::string historyTargetName;
  std::string inspectPendingTargetName;
  int inspectPendingReturnPage = 3;
  nlohmann::json historyPayload;
  std::string historySelectedMatchId;
  nlohmann::json historySelectedMatchDetail;
  nlohmann::json profileHistoryPayload;
  bool profileEditMode = false;
  bool profileAutoLoadDone = false;
  std::int64_t profileAutoLoadUserId = 0;

  std::string lastResponse;
  std::vector<std::string> logs;

  std::unordered_map<std::string, std::shared_future<nlohmann::json>> asyncFutureByKey;
  std::unordered_map<std::string, std::chrono::steady_clock::time_point> asyncDeadlineByKey;
  std::unordered_map<std::string, nlohmann::json> asyncResponseByKey;
  bool networkTimeoutPopupOpen = false;
  std::string networkTimeoutPopupMessage;
};

}  // namespace simpleelo::client::ui
