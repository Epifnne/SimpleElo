#include "serverEngine.h"

#include "protocol/api.h"

namespace simpleelo::server {
namespace {

using nlohmann::json;

std::string jsonErr(int code, const std::string& message) {
  return json({{"code", code}, {"message", message}}).dump();
}

}  // namespace

std::string ServerEngine::dispatchAction(const std::string& action, const nlohmann::json& req) {
  if (action == protocol::api::kActionPing) {
    return json({{"code", 0}, {"message", "pong"}, {"serverTime", nowIso8601()}}).dump();
  }

  if (action == protocol::api::kActionSendCode) return handleSendCode(req);
  if (action == protocol::api::kActionRegister) return handleRegister(req);
  if (action == protocol::api::kActionLogin) return handleLogin(req);
  if (action == protocol::api::kActionResetPassword) return handleResetPassword(req);
  if (action == protocol::api::kActionCreateRoom) return handleCreateRoom(req);
  if (action == protocol::api::kActionListRooms) return handleListRooms(req);
  if (action == protocol::api::kActionJoinRoom) return handleJoinRoom(req);
  if (action == protocol::api::kActionLeaveRoom) return handleLeaveRoom(req);
  if (action == protocol::api::kActionGetRoomDetail) return handleGetRoomDetail(req);
  if (action == protocol::api::kActionSetPlayer) return handleSetPlayer(req);
  if (action == protocol::api::kActionSetRoomBp) return handleSetRoomBp(req);
  if (action == protocol::api::kActionStartGame) return handleStartGame(req);
  if (action == protocol::api::kActionAbortGame) return handleAbortGame(req);
  if (action == protocol::api::kActionDeleteRoom) return handleDeleteRoom(req);
  if (action == protocol::api::kActionSubmitMatch) return handleSubmitMatch(req);
  if (action == protocol::api::kActionVote) return handleVote(req);
  if (action == protocol::api::kActionGetMatch) return handleGetMatch(req);
  if (action == protocol::api::kActionGetProfile) return handleGetProfile(req);
  if (action == protocol::api::kActionGetUserHistory) return handleGetUserHistory(req);

  return jsonErr(499, "unknown action");
}

}  // namespace simpleelo::server
