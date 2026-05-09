#pragma once

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

#include "protocol/messages.h"

namespace simpleelo::protocol::api {

#if !defined(SIMPLEELO_API_CLIENT) && !defined(SIMPLEELO_API_SERVER)
#define SIMPLEELO_API_CLIENT
#define SIMPLEELO_API_SERVER
#endif

inline constexpr const char* kActionSendCode = "sendCode";
inline constexpr const char* kActionPing = "ping";
inline constexpr const char* kActionRegister = "register";
inline constexpr const char* kActionLogin = "login";
inline constexpr const char* kActionResetPassword = "resetPassword";
inline constexpr const char* kActionCreateRoom = "createRoom";
inline constexpr const char* kActionListRooms = "listRooms";
inline constexpr const char* kActionJoinRoom = "joinRoom";
inline constexpr const char* kActionLeaveRoom = "leaveRoom";
inline constexpr const char* kActionGetRoomDetail = "getRoomDetail";
inline constexpr const char* kActionSetPlayer = "setPlayer";
inline constexpr const char* kActionSetRoomBp = "setRoomBp";
inline constexpr const char* kActionStartGame = "startGame";
inline constexpr const char* kActionAbortGame = "abortGame";
inline constexpr const char* kActionDeleteRoom = "deleteRoom";
inline constexpr const char* kActionSubmitMatch = "submitMatch";
inline constexpr const char* kActionVote = "vote";
inline constexpr const char* kActionGetMatch = "getMatch";
inline constexpr const char* kActionGetProfile = "getProfile";
inline constexpr const char* kActionGetUserHistory = "getUserHistory";

struct SendCodeRequest {
  std::string email;
};

struct CreateRoomRequest {
  std::string token;
  std::string roomName;
  std::string roomPassword;
};

struct JoinRoomRequest {
  std::string token;
  std::string roomId;
  std::string roomPassword;
};

struct ResetPasswordRequest {
  std::string email;
  std::string verifyCode;
  std::string newPassword;
};

#if defined(SIMPLEELO_API_CLIENT) || defined(SIMPLEELO_API_SERVER)
inline ApiResponse validateSendCodeRequest(const SendCodeRequest& request) {
  if (request.email.empty()) {
    return {400, "email is required"};
  }
  return {0, "ok"};
}

inline ApiResponse validateRegisterRequest(const RegisterRequest& request) {
  if (request.email.empty() || request.password.empty() || request.verifyCode.empty()) {
    return {401, "email/password/verifyCode required"};
  }
  return {0, "ok"};
}

inline ApiResponse validateLoginRequest(const LoginRequest& request) {
  if (request.email.empty() || request.password.empty()) {
    return {411, "email/password required"};
  }
  return {0, "ok"};
}

inline ApiResponse validateResetPasswordRequest(const ResetPasswordRequest& request) {
  if (request.email.empty() || request.verifyCode.empty() || request.newPassword.empty()) {
    return {414, "email/verifyCode/newPassword required"};
  }
  return {0, "ok"};
}

inline ApiResponse validateCreateRoomRequest(const CreateRoomRequest& request) {
  if (request.token.empty()) {
    return {421, "invalid token"};
  }
  return {0, "ok"};
}

inline ApiResponse validateJoinRoomRequest(const JoinRoomRequest& request) {
  if (request.token.empty()) {
    return {431, "invalid token"};
  }
  if (request.roomId.empty()) {
    return {432, "room not found"};
  }
  return {0, "ok"};
}

#endif

#ifdef SIMPLEELO_API_SERVER
inline ApiResponse validateSubmitMatchRequest(const SubmitMatchRequest& request) {
  if (request.roomId.empty() || request.idempotencyKey.empty() || request.ownerUserId <= 0) {
    return {452, "roomId/idempotencyKey required"};
  }
  return {0, "ok"};
}

inline ApiResponse validateVoteRequest(const VoteRequest& request) {
  if (request.voterUserId <= 0) {
    return {461, "invalid token"};
  }
  if (request.matchId.empty()) {
    return {462, "match not found"};
  }
  return {0, "ok"};
}
#endif



#ifdef SIMPLEELO_API_CLIENT
inline nlohmann::json buildPingRequest() {
  return {{"action", kActionPing}};
}

inline nlohmann::json buildSendCodeRequest(const SendCodeRequest& request) {
  return {{"action", kActionSendCode}, {"email", request.email}};
}

inline nlohmann::json buildRegisterRequest(const RegisterRequest& request, const std::string& nickname) {
  return {{"action", kActionRegister},
          {"email", request.email},
          {"password", request.password},
          {"nickname", nickname},
          {"verifyCode", request.verifyCode}};
}

inline nlohmann::json buildLoginRequest(const LoginRequest& request) {
  return {{"action", kActionLogin}, {"email", request.email}, {"password", request.password}};
}

inline nlohmann::json buildResetPasswordRequest(const ResetPasswordRequest& request) {
  return {{"action", kActionResetPassword},
          {"email", request.email},
          {"verifyCode", request.verifyCode},
          {"newPassword", request.newPassword}};
}

inline nlohmann::json buildCreateRoomRequest(const CreateRoomRequest& request) {
  return {{"action", kActionCreateRoom},
          {"token", request.token},
          {"roomName", request.roomName},
          {"roomPassword", request.roomPassword}};
}

inline nlohmann::json buildListRoomsRequest(const std::string& token) {
  return {{"action", kActionListRooms}, {"token", token}};
}

inline nlohmann::json buildJoinRoomRequest(const JoinRoomRequest& request) {
  return {{"action", kActionJoinRoom},
          {"token", request.token},
          {"roomId", request.roomId},
          {"roomPassword", request.roomPassword}};
}

inline nlohmann::json buildLeaveRoomRequest(const std::string& token, const std::string& roomId) {
  return {{"action", kActionLeaveRoom}, {"token", token}, {"roomId", roomId}};
}

inline nlohmann::json buildSetRoomBpRequest(const std::string& token, const std::string& roomId, bool bpEnabled) {
  return {{"action", kActionSetRoomBp}, {"token", token}, {"roomId", roomId}, {"bpEnabled", bpEnabled}};
}

inline nlohmann::json buildStartGameRequest(const std::string& token, const std::string& roomId) {
  return {{"action", kActionStartGame}, {"token", token}, {"roomId", roomId}};
}

inline nlohmann::json buildAbortGameRequest(const std::string& token, const std::string& roomId) {
  return {{"action", kActionAbortGame}, {"token", token}, {"roomId", roomId}};
}

inline nlohmann::json buildDeleteRoomRequest(const std::string& token, const std::string& roomId) {
  return {{"action", kActionDeleteRoom}, {"token", token}, {"roomId", roomId}};
}

inline nlohmann::json buildGetRoomDetailRequest(const std::string& token, const std::string& roomId) {
  return {{"action", kActionGetRoomDetail}, {"token", token}, {"roomId", roomId}};
}

inline nlohmann::json buildGetProfileRequest(const std::string& token) {
  return {{"action", kActionGetProfile}, {"token", token}};
}

inline nlohmann::json buildGetUserHistoryRequest(const std::string& token, std::int64_t userId) {
  return {{"action", kActionGetUserHistory}, {"token", token}, {"userId", userId}};
}

inline nlohmann::json buildGetMatchRequest(const std::string& token, const std::string& matchId) {
  return {{"action", kActionGetMatch}, {"token", token}, {"matchId", matchId}};
}
#endif

}  // namespace simpleelo::protocol::api
