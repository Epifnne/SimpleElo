#pragma once

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <nlohmann/json.hpp>

namespace simpleelo::server {

struct UserRecord {
  std::int64_t userId = 0;
  std::string email;
  std::string nickname;
  std::string passwordHash;
  int elo = 1000;
  int wins = 0;
  int losses = 0;
};

struct PlayerState {
  std::int64_t userId = 0;
  std::string team = "red";
  std::string role = "";
  int roleScore = 0;
  bool ready = false;
  bool online = true;
  bool leftEarly = false;
};

struct RoomRecord {
  std::string roomId;
  std::string roomName;
  std::string roomPassword;
  std::int64_t ownerUserId = 0;
  bool locked = false;
  bool gameStarted = false;
  bool bpEnabled = false;
  std::unordered_map<std::int64_t, PlayerState> players;
};

struct MatchRecord {
  std::string matchId;
  std::string roomId;
  std::int64_t ownerUserId = 0;
  std::string winner = "red";
  std::string idempotencyKey;
  std::int64_t createdAtEpochSec = 0;
  std::int64_t voteDeadlineEpochSec = 0;
  bool finalized = false;
  bool passed = false;
  std::unordered_set<std::int64_t> approved;
  std::unordered_set<std::int64_t> rejected;
  std::unordered_set<std::int64_t> effectiveVoters;
  nlohmann::json snapshotPlayers = nlohmann::json::array();
};

struct MatchHistoryRecord {
  std::string matchId;
  std::string roomId;
  std::int64_t userId = 0;
  std::int64_t createdAtEpochSec = 0;
  std::string role;
  int eloBefore = 1000;
  int eloAfter = 1000;
  int delta = 0;
  std::string outcome;
};

class ServerEngine {
 public:
  explicit ServerEngine(std::string dataFilePath);

  bool load();
  bool save();

  std::string handleRequest(const std::string& payload);

 private:
  std::string dispatchAction(const std::string& action, const nlohmann::json& req);

  std::string makeToken(std::int64_t userId) const;
  std::int64_t verifyToken(const std::string& token) const;
  std::string hashPassword(const std::string& password) const;

  void finalizeMatchIfNeeded(MatchRecord& match, bool forceDeadlineCheck);
  nlohmann::json buildMatchStatus(const MatchRecord& match) const;

  std::string handleSendCode(const nlohmann::json& req);
  std::string handleRegister(const nlohmann::json& req);
  std::string handleLogin(const nlohmann::json& req);
  std::string handleResetPassword(const nlohmann::json& req);
  std::string handleCreateRoom(const nlohmann::json& req);
  std::string handleListRooms(const nlohmann::json& req);
  std::string handleJoinRoom(const nlohmann::json& req);
  std::string handleLeaveRoom(const nlohmann::json& req);
  std::string handleGetRoomDetail(const nlohmann::json& req);
  std::string handleSetPlayer(const nlohmann::json& req);
  std::string handleSetRoomBp(const nlohmann::json& req);
  std::string handleStartGame(const nlohmann::json& req);
  std::string handleAbortGame(const nlohmann::json& req);
  std::string handleDeleteRoom(const nlohmann::json& req);
  std::string handleSubmitMatch(const nlohmann::json& req);
  std::string handleVote(const nlohmann::json& req);
  std::string handleGetMatch(const nlohmann::json& req);
  std::string handleGetProfile(const nlohmann::json& req);
  std::string handleGetUserHistory(const nlohmann::json& req);

  static std::string nowIso8601();
  static std::int64_t nowEpochSec();

  mutable std::mutex mutex_;
  std::string dataFilePath_;

  std::int64_t nextUserId_ = 1;
  std::int64_t nextRoomCounter_ = 1;

  std::unordered_map<std::int64_t, UserRecord> usersById_;
  std::unordered_map<std::string, std::int64_t> userIdByEmail_;
  std::unordered_map<std::string, std::string> verifyCodeByEmail_;
  std::unordered_map<std::string, std::int64_t> tokenToUserId_;

  std::unordered_map<std::string, RoomRecord> roomsById_;
  std::unordered_set<std::string> deletedRoomIds_;
  std::unordered_map<std::string, MatchRecord> matchesById_;
  std::unordered_map<std::string, std::string> matchIdByIdempotencyKey_;
  std::vector<MatchHistoryRecord> matchHistory_;
};

}  // namespace simpleelo::server
