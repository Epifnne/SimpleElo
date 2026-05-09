#include "serverEngine.h"

#include "elo/eloCalculator.h"
#include "protocol/api.h"

namespace simpleelo::server {
namespace {

using nlohmann::json;

std::string jsonOk(const json& extra = {}) {
  json r = {{"code", 0}, {"message", "ok"}};
  for (const auto& [k, v] : extra.items()) {
    r[k] = v;
  }
  return r.dump();
}

std::string jsonErr(int code, const std::string& message) {
  return json({{"code", code}, {"message", message}}).dump();
}

json userToJson(const UserRecord& u) {
  return {{"userId", u.userId}, {"email", u.email}, {"nickname", u.nickname}, {"elo", u.elo}, {"wins", u.wins}, {"losses", u.losses}};
}

}  // namespace

std::string ServerEngine::handleSendCode(const nlohmann::json& req) {
  const protocol::api::SendCodeRequest request{req.value("email", "")};
  const auto validation = protocol::api::validateSendCodeRequest(request);
  if (validation.code != 0) {
    return jsonErr(validation.code, validation.message);
  }
  verifyCodeByEmail_[request.email] = "123456";
  save();
  return jsonOk({{"verifyCode", "123456"}, {"note", "dev-mode fixed code"}});
}

std::string ServerEngine::handleRegister(const nlohmann::json& req) {
  const protocol::RegisterRequest request{
      req.value("email", ""),
      req.value("password", ""),
      req.value("verifyCode", "")};
  const std::string nicknameReq = req.value("nickname", "");
  const auto validation = protocol::api::validateRegisterRequest(request);
  if (validation.code != 0) {
    return jsonErr(validation.code, validation.message);
  }
  const std::string& email = request.email;
  if (userIdByEmail_.find(email) != userIdByEmail_.end()) {
    return jsonErr(402, "email already registered");
  }
  if (verifyCodeByEmail_[email] != request.verifyCode) {
    return jsonErr(403, "verify code mismatch");
  }

  UserRecord u;
  u.userId = nextUserId_++;
  u.email = email;
  if (!nicknameReq.empty()) {
    u.nickname = nicknameReq;
  } else {
    const auto at = email.find('@');
    u.nickname = (at == std::string::npos) ? email : email.substr(0, at);
  }
  u.passwordHash = hashPassword(request.password);
  usersById_[u.userId] = u;
  userIdByEmail_[email] = u.userId;

  save();
  return jsonOk({{"user", userToJson(u)}});
}

std::string ServerEngine::handleLogin(const nlohmann::json& req) {
  const protocol::LoginRequest request{req.value("email", ""), req.value("password", "")};
  const auto validation = protocol::api::validateLoginRequest(request);
  if (validation.code != 0) {
    return jsonErr(validation.code, validation.message);
  }
  const std::string& email = request.email;
  if (userIdByEmail_.find(email) == userIdByEmail_.end()) {
    return jsonErr(411, "account not found");
  }
  auto userIt = usersById_.find(userIdByEmail_[email]);
  if (userIt == usersById_.end()) {
    return jsonErr(412, "account broken");
  }
  if (userIt->second.passwordHash != hashPassword(request.password)) {
    return jsonErr(413, "password invalid");
  }

  const std::string token = makeToken(userIt->second.userId);
  tokenToUserId_[token] = userIt->second.userId;
  save();
  return jsonOk({{"token", token}, {"user", userToJson(userIt->second)}});
}

std::string ServerEngine::handleResetPassword(const nlohmann::json& req) {
  const protocol::api::ResetPasswordRequest request{
      req.value("email", ""),
      req.value("verifyCode", ""),
      req.value("newPassword", "")};
  const auto validation = protocol::api::validateResetPasswordRequest(request);
  if (validation.code != 0) {
    return jsonErr(validation.code, validation.message);
  }

  const auto uidIt = userIdByEmail_.find(request.email);
  if (uidIt == userIdByEmail_.end()) {
    return jsonErr(415, "account not found");
  }
  const auto codeIt = verifyCodeByEmail_.find(request.email);
  if (codeIt == verifyCodeByEmail_.end() || codeIt->second != request.verifyCode) {
    return jsonErr(416, "verify code mismatch");
  }

  auto userIt = usersById_.find(uidIt->second);
  if (userIt == usersById_.end()) {
    return jsonErr(417, "account broken");
  }

  userIt->second.passwordHash = hashPassword(request.newPassword);
  save();
  return jsonOk({{"user", userToJson(userIt->second)}});
}

std::string ServerEngine::handleCreateRoom(const nlohmann::json& req) {
  const protocol::api::CreateRoomRequest request{
      req.value("token", ""),
      req.value("roomName", "Simple Room"),
      req.value("roomPassword", "")};
  const auto validation = protocol::api::validateCreateRoomRequest(request);
  if (validation.code != 0) {
    return jsonErr(validation.code, validation.message);
  }
  const std::int64_t userId = verifyToken(request.token);
  if (userId == 0) {
    return jsonErr(421, "invalid token");
  }

  RoomRecord room;
  room.roomId = "room-" + std::to_string(nextRoomCounter_++);
  room.roomName = request.roomName;
  room.roomPassword = request.roomPassword;
  room.ownerUserId = userId;
  room.locked = false;
  room.gameStarted = false;
  room.bpEnabled = false;
  room.players[userId] = PlayerState{userId, "red", "captain", 0, false, true, false};

  roomsById_[room.roomId] = room;
  save();
  return jsonOk({{"roomId", room.roomId}, {"roomName", room.roomName}});
}

std::string ServerEngine::handleListRooms(const nlohmann::json& req) {
  const std::string token = req.value("token", "");
  const std::int64_t userId = verifyToken(token);
  if (userId == 0) {
    return jsonErr(425, "invalid token");
  }

  json rooms = json::array();
  for (const auto& [roomId, room] : roomsById_) {
    (void)roomId;
    rooms.push_back({
        {"roomId", room.roomId},
        {"roomName", room.roomName},
        {"ownerUserId", room.ownerUserId},
        {"memberCount", static_cast<int>(room.players.size())},
        {"hasPassword", !room.roomPassword.empty()},
        {"locked", room.locked},
        {"gameStarted", room.gameStarted},
        {"bpEnabled", room.bpEnabled},
    });
  }
  return jsonOk({{"rooms", rooms}});
}

std::string ServerEngine::handleJoinRoom(const nlohmann::json& req) {
  const protocol::api::JoinRoomRequest request{
      req.value("token", ""),
      req.value("roomId", ""),
      req.value("roomPassword", "")};
  const auto validation = protocol::api::validateJoinRoomRequest(request);
  if (validation.code != 0) {
    return jsonErr(validation.code, validation.message);
  }
  const std::int64_t userId = verifyToken(request.token);
  const std::string& roomId = request.roomId;
  if (userId == 0) {
    return jsonErr(431, "invalid token");
  }
  auto roomIt = roomsById_.find(roomId);
  if (roomIt == roomsById_.end()) {
    if (deletedRoomIds_.find(roomId) != deletedRoomIds_.end()) {
      return jsonErr(438, "room has been deleted");
    }
    return jsonErr(432, "room not found");
  }

  const bool alreadyMember = roomIt->second.players.find(userId) != roomIt->second.players.end();
  if (roomIt->second.locked && !alreadyMember) {
    return jsonErr(434, "room locked");
  }

  if (!roomIt->second.roomPassword.empty() && roomIt->second.roomPassword != request.roomPassword) {
    return jsonErr(433, "room password invalid");
  }

  if (!alreadyMember) {
    const std::string team = (roomIt->second.players.size() % 2 == 0) ? "red" : "blue";
    roomIt->second.players[userId] = PlayerState{userId, team, "", 0, false, true, false};
  } else {
    auto& ps = roomIt->second.players[userId];
    ps.online = true;
    ps.leftEarly = false;
  }
  save();
  return jsonOk({{"roomId", roomId}, {"memberCount", static_cast<int>(roomIt->second.players.size())}});
}

std::string ServerEngine::handleLeaveRoom(const nlohmann::json& req) {
  const std::string token = req.value("token", "");
  const std::int64_t userId = verifyToken(token);
  const std::string roomId = req.value("roomId", "");
  if (userId == 0) {
    return jsonErr(439, "invalid token");
  }
  auto roomIt = roomsById_.find(roomId);
  if (roomIt == roomsById_.end()) {
    if (deletedRoomIds_.find(roomId) != deletedRoomIds_.end()) {
      return jsonErr(438, "room has been deleted");
    }
    return jsonErr(432, "room not found");
  }
  auto playerIt = roomIt->second.players.find(userId);
  if (playerIt == roomIt->second.players.end()) {
    return jsonErr(443, "not in room");
  }

  if (roomIt->second.locked || roomIt->second.gameStarted) {
    playerIt->second.online = false;
    playerIt->second.leftEarly = true;
    playerIt->second.ready = false;
    save();
    return jsonOk({{"roomId", roomId}, {"leftAsOffline", true}});
  }

  roomIt->second.players.erase(playerIt);
  if (roomIt->second.players.empty()) {
    roomsById_.erase(roomIt);
    save();
    return jsonOk({{"roomId", roomId}, {"deleted", true}});
  }

  if (roomIt->second.ownerUserId == userId) {
    roomIt->second.ownerUserId = roomIt->second.players.begin()->first;
  }
  save();
  return jsonOk({{"roomId", roomId}, {"leftAsOffline", false}});
}

std::string ServerEngine::handleGetRoomDetail(const nlohmann::json& req) {
  const std::string token = req.value("token", "");
  const std::int64_t userId = verifyToken(token);
  const std::string roomId = req.value("roomId", "");
  if (userId == 0) {
    return jsonErr(436, "invalid token");
  }
  auto roomIt = roomsById_.find(roomId);
  if (roomIt == roomsById_.end()) {
    if (deletedRoomIds_.find(roomId) != deletedRoomIds_.end()) {
      return jsonErr(438, "room has been deleted");
    }
    return jsonErr(437, "room not found");
  }

  int redTotal = 0;
  int blueTotal = 0;
  int redCount = 0;
  int blueCount = 0;
  for (const auto& [uid, p] : roomIt->second.players) {
    (void)uid;
    auto userIt = usersById_.find(p.userId);
    if (userIt == usersById_.end()) {
      continue;
    }
    if (p.team == "blue") {
      blueTotal += userIt->second.elo;
      ++blueCount;
    } else {
      redTotal += userIt->second.elo;
      ++redCount;
    }
  }

  int redDelta = 0;
  int blueDelta = 0;
  if (redCount > 0 && blueCount > 0) {
    const int redAvg = redTotal / redCount;
    const int blueAvg = blueTotal / blueCount;
    const auto [redWinAfter, blueLoseAfter] = simpleelo::elo::updateTeamElo(redAvg, blueAvg, simpleelo::elo::EloConfig{});
    redDelta = redWinAfter - redAvg;
    blueDelta = blueLoseAfter - blueAvg;
  }

  json teamRed = json::array();
  json teamBlue = json::array();
  for (const auto& [uid, p] : roomIt->second.players) {
    (void)uid;
    auto userIt = usersById_.find(p.userId);
    if (userIt == usersById_.end()) {
      continue;
    }
    json row = {
        {"userId", p.userId},
        {"nickname", userIt->second.nickname.empty() ? userIt->second.email : userIt->second.nickname},
        {"elo", userIt->second.elo},
        {"role", p.role},
        {"roleScore", p.roleScore},
        {"ready", p.ready},
        {"online", p.online},
        {"leftEarly", p.leftEarly},
        {"expectedWinDelta", p.team == "blue" ? -blueDelta : redDelta},
        {"expectedLoseDelta", p.team == "blue" ? blueDelta : -redDelta},
    };
    if (p.team == "blue") {
      teamBlue.push_back(row);
    } else {
      teamRed.push_back(row);
    }
  }

  return jsonOk({
      {"room", {{"roomId", roomIt->second.roomId},
                {"roomName", roomIt->second.roomName},
                {"ownerUserId", roomIt->second.ownerUserId},
                {"locked", roomIt->second.locked},
                {"gameStarted", roomIt->second.gameStarted},
                {"bpEnabled", roomIt->second.bpEnabled}}},
      {"predicted", {{"redWinDelta", redDelta}, {"blueWinDelta", -blueDelta}}},
      {"teamRed", teamRed},
      {"teamBlue", teamBlue},
  });
}

std::string ServerEngine::handleSetPlayer(const nlohmann::json& req) {
  const std::string token = req.value("token", "");
  const std::int64_t userId = verifyToken(token);
  const std::string roomId = req.value("roomId", "");
  if (userId == 0) {
    return jsonErr(441, "invalid token");
  }
  auto roomIt = roomsById_.find(roomId);
  if (roomIt == roomsById_.end()) {
    return jsonErr(442, "room not found");
  }
  auto playerIt = roomIt->second.players.find(userId);
  if (playerIt == roomIt->second.players.end()) {
    return jsonErr(443, "not in room");
  }

  playerIt->second.team = req.value("team", playerIt->second.team);
  playerIt->second.role = req.value("role", playerIt->second.role);
  playerIt->second.roleScore = req.value("roleScore", playerIt->second.roleScore);
  playerIt->second.ready = req.value("ready", playerIt->second.ready);
  playerIt->second.online = true;
  playerIt->second.leftEarly = req.value("leftEarly", playerIt->second.leftEarly);

  save();
  return jsonOk({{"roomId", roomId},
                 {"team", playerIt->second.team},
                 {"role", playerIt->second.role},
                 {"roleScore", playerIt->second.roleScore},
                 {"ready", playerIt->second.ready},
                 {"online", playerIt->second.online},
                 {"leftEarly", playerIt->second.leftEarly}});
}

std::string ServerEngine::handleSetRoomBp(const nlohmann::json& req) {
  const std::string token = req.value("token", "");
  const std::int64_t userId = verifyToken(token);
  const std::string roomId = req.value("roomId", "");
  const bool bpEnabled = req.value("bpEnabled", false);
  if (userId == 0) {
    return jsonErr(441, "invalid token");
  }
  auto roomIt = roomsById_.find(roomId);
  if (roomIt == roomsById_.end()) {
    return jsonErr(442, "room not found");
  }
  if (roomIt->second.ownerUserId != userId) {
    return jsonErr(444, "only owner can change bp");
  }
  roomIt->second.bpEnabled = bpEnabled;
  save();
  return jsonOk({{"roomId", roomId}, {"bpEnabled", roomIt->second.bpEnabled}});
}

std::string ServerEngine::handleStartGame(const nlohmann::json& req) {
  const std::string token = req.value("token", "");
  const std::int64_t userId = verifyToken(token);
  const std::string roomId = req.value("roomId", "");
  if (userId == 0) {
    return jsonErr(441, "invalid token");
  }
  auto roomIt = roomsById_.find(roomId);
  if (roomIt == roomsById_.end()) {
    return jsonErr(442, "room not found");
  }
  if (roomIt->second.ownerUserId != userId) {
    return jsonErr(445, "only owner can start game");
  }

  int onlineCount = 0;
  for (const auto& [uid, p] : roomIt->second.players) {
    (void)uid;
    if (!p.online) {
      continue;
    }
    ++onlineCount;
    if (!p.ready) {
      return jsonErr(446, "all online players must be ready");
    }
    if (p.role.empty()) {
      return jsonErr(447, "all online players must choose role");
    }
  }
  if (onlineCount == 0) {
    return jsonErr(448, "no online players");
  }

  roomIt->second.locked = true;
  roomIt->second.gameStarted = true;
  save();
  return jsonOk({{"roomId", roomId}, {"locked", true}, {"gameStarted", true}});
}

std::string ServerEngine::handleAbortGame(const nlohmann::json& req) {
  const std::string token = req.value("token", "");
  const std::int64_t userId = verifyToken(token);
  const std::string roomId = req.value("roomId", "");
  if (userId == 0) {
    return jsonErr(441, "invalid token");
  }
  auto roomIt = roomsById_.find(roomId);
  if (roomIt == roomsById_.end()) {
    return jsonErr(442, "room not found");
  }
  if (roomIt->second.ownerUserId != userId) {
    return jsonErr(449, "only owner can abort game");
  }

  roomIt->second.gameStarted = false;
  roomIt->second.locked = false;
  for (auto& [uid, p] : roomIt->second.players) {
    (void)uid;
    p.ready = false;
  }
  save();
  return jsonOk({{"roomId", roomId}, {"locked", false}, {"gameStarted", false}});
}

std::string ServerEngine::handleDeleteRoom(const nlohmann::json& req) {
  const std::string token = req.value("token", "");
  const std::int64_t userId = verifyToken(token);
  const std::string roomId = req.value("roomId", "");
  if (userId == 0) {
    return jsonErr(441, "invalid token");
  }
  auto roomIt = roomsById_.find(roomId);
  if (roomIt == roomsById_.end()) {
    return jsonErr(442, "room not found");
  }
  if (roomIt->second.ownerUserId != userId) {
    return jsonErr(450, "only owner can delete room");
  }

  roomsById_.erase(roomIt);
  deletedRoomIds_.insert(roomId);
  save();
  return jsonOk({{"roomId", roomId}, {"deleted", true}, {"message", "room has been deleted"}});
}

std::string ServerEngine::handleGetProfile(const nlohmann::json& req) {
  const std::string token = req.value("token", "");
  const std::int64_t userId = verifyToken(token);
  if (userId == 0) {
    return jsonErr(481, "invalid token");
  }
  auto userIt = usersById_.find(userId);
  if (userIt == usersById_.end()) {
    return jsonErr(482, "user not found");
  }

  json history = json::array();
  for (const auto& h : matchHistory_) {
    if (h.userId == userId) {
      history.push_back({
          {"matchId", h.matchId},
          {"roomId", h.roomId},
          {"createdAtEpochSec", h.createdAtEpochSec},
          {"role", h.role},
          {"eloBefore", h.eloBefore},
          {"eloAfter", h.eloAfter},
          {"delta", h.delta},
          {"outcome", h.outcome},
      });
    }
  }

  return jsonOk({{"user", userToJson(userIt->second)}, {"history", history}});
}

std::string ServerEngine::handleGetUserHistory(const nlohmann::json& req) {
  const std::string token = req.value("token", "");
  const std::int64_t requesterId = verifyToken(token);
  const std::int64_t targetUserId = req.value("userId", 0LL);
  if (requesterId == 0) {
    return jsonErr(486, "invalid token");
  }
  auto userIt = usersById_.find(targetUserId);
  if (userIt == usersById_.end()) {
    return jsonErr(487, "target user not found");
  }

  json history = json::array();
  for (const auto& h : matchHistory_) {
    if (h.userId == targetUserId) {
      history.push_back({
          {"matchId", h.matchId},
          {"roomId", h.roomId},
          {"createdAtEpochSec", h.createdAtEpochSec},
          {"role", h.role},
          {"eloBefore", h.eloBefore},
          {"eloAfter", h.eloAfter},
          {"delta", h.delta},
          {"outcome", h.outcome},
      });
    }
  }
  return jsonOk({{"user", userToJson(userIt->second)}, {"history", history}});
}

}  // namespace simpleelo::server
