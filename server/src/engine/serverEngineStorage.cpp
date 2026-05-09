#include "serverEngine.h"

#include <fstream>

namespace simpleelo::server {
namespace {

using nlohmann::json;

}  // namespace

bool ServerEngine::load() {
  std::scoped_lock lock(mutex_);
  try {
    std::ifstream input(dataFilePath_);
    if (!input.good()) {
      return true;
    }

    json root;
    input >> root;

    nextUserId_ = root.value("nextUserId", 1);
    nextRoomCounter_ = root.value("nextRoomCounter", 1);

    usersById_.clear();
    userIdByEmail_.clear();
    for (const auto& v : root.value("users", json::array())) {
      UserRecord u;
      u.userId = v.value("userId", 0);
      u.email = v.value("email", "");
      u.nickname = v.value("nickname", "");
      u.passwordHash = v.value("passwordHash", "");
      u.elo = v.value("elo", 1000);
      u.wins = v.value("wins", 0);
      u.losses = v.value("losses", 0);
      usersById_[u.userId] = u;
      if (!u.email.empty()) {
        userIdByEmail_[u.email] = u.userId;
      }
    }

    verifyCodeByEmail_.clear();
    for (const auto& [email, code] : root.value("verifyCodes", json::object()).items()) {
      if (!code.is_null()) {
        verifyCodeByEmail_[email] = code.get<std::string>();
      }
    }

    tokenToUserId_.clear();
    for (const auto& [token, userId] : root.value("sessions", json::object()).items()) {
      if (!userId.is_null()) {
        tokenToUserId_[token] = userId.get<std::int64_t>();
      }
    }

    roomsById_.clear();
    deletedRoomIds_.clear();
    for (const auto& roomJson : root.value("rooms", json::array())) {
      RoomRecord room;
      room.roomId = roomJson.value("roomId", "");
      room.roomName = roomJson.value("roomName", "");
      room.roomPassword = roomJson.value("roomPassword", "");
      room.ownerUserId = roomJson.value("ownerUserId", 0LL);
      room.locked = roomJson.value("locked", false);
      room.gameStarted = roomJson.value("gameStarted", false);
      room.bpEnabled = roomJson.value("bpEnabled", false);
      for (const auto& p : roomJson.value("players", json::array())) {
        PlayerState ps;
        ps.userId = p.value("userId", 0LL);
        ps.team = p.value("team", "red");
        ps.role = p.value("role", "");
        ps.roleScore = p.value("roleScore", 0);
        ps.ready = p.value("ready", false);
        ps.online = p.value("online", true);
        ps.leftEarly = p.value("leftEarly", false);
        room.players[ps.userId] = ps;
      }
      if (!room.roomId.empty()) {
        roomsById_[room.roomId] = room;
      }
    }

    for (const auto& roomId : root.value("deletedRooms", json::array())) {
      if (roomId.is_string()) {
        deletedRoomIds_.insert(roomId.get<std::string>());
      }
    }

    matchesById_.clear();
    matchIdByIdempotencyKey_.clear();
    for (const auto& m : root.value("matches", json::array())) {
      MatchRecord mr;
      mr.matchId = m.value("matchId", "");
      mr.roomId = m.value("roomId", "");
      mr.ownerUserId = m.value("ownerUserId", 0LL);
      mr.winner = m.value("winner", "red");
      mr.idempotencyKey = m.value("idempotencyKey", "");
      mr.createdAtEpochSec = m.value("createdAtEpochSec", 0LL);
      mr.voteDeadlineEpochSec = m.value("voteDeadlineEpochSec", 0LL);
      mr.finalized = m.value("finalized", false);
      mr.passed = m.value("passed", false);
      mr.snapshotPlayers = m.value("snapshotPlayers", json::array());
      for (const auto& v : m.value("approved", json::array())) mr.approved.insert(v.get<std::int64_t>());
      for (const auto& v : m.value("rejected", json::array())) mr.rejected.insert(v.get<std::int64_t>());
      for (const auto& v : m.value("effectiveVoters", json::array())) mr.effectiveVoters.insert(v.get<std::int64_t>());
      if (!mr.matchId.empty()) {
        matchesById_[mr.matchId] = mr;
      }
      if (!mr.idempotencyKey.empty()) {
        matchIdByIdempotencyKey_[mr.idempotencyKey] = mr.matchId;
      }
    }

    matchHistory_.clear();
    for (const auto& h : root.value("matchHistory", json::array())) {
      MatchHistoryRecord hr;
      hr.matchId = h.value("matchId", "");
      hr.roomId = h.value("roomId", "");
      hr.userId = h.value("userId", 0LL);
      hr.createdAtEpochSec = h.value("createdAtEpochSec", 0LL);
      hr.role = h.value("role", std::string{});
      hr.eloBefore = h.value("eloBefore", 1000);
      hr.eloAfter = h.value("eloAfter", 1000);
      hr.delta = h.value("delta", 0);
      hr.outcome = h.value("outcome", "");
      matchHistory_.push_back(hr);
    }

    return true;
  } catch (const std::exception&) {
    usersById_.clear();
    userIdByEmail_.clear();
    verifyCodeByEmail_.clear();
    tokenToUserId_.clear();
    roomsById_.clear();
    deletedRoomIds_.clear();
    matchesById_.clear();
    matchIdByIdempotencyKey_.clear();
    matchHistory_.clear();
    nextUserId_ = 1;
    nextRoomCounter_ = 1;
    return false;
  }
}

bool ServerEngine::save() {
  json root;

  root["nextUserId"] = nextUserId_;
  root["nextRoomCounter"] = nextRoomCounter_;

  root["users"] = json::array();
  for (const auto& [id, user] : usersById_) {
    (void)id;
    root["users"].push_back({
        {"userId", user.userId},
        {"email", user.email},
        {"nickname", user.nickname},
        {"passwordHash", user.passwordHash},
        {"elo", user.elo},
        {"wins", user.wins},
        {"losses", user.losses},
    });
  }

  root["verifyCodes"] = json::object();
  for (const auto& [email, code] : verifyCodeByEmail_) {
    root["verifyCodes"][email] = code;
  }

  root["sessions"] = json::object();
  for (const auto& [token, userId] : tokenToUserId_) {
    root["sessions"][token] = userId;
  }

  root["rooms"] = json::array();
  for (const auto& [roomId, room] : roomsById_) {
    (void)roomId;
    json roomJson = {
        {"roomId", room.roomId},
        {"roomName", room.roomName},
        {"roomPassword", room.roomPassword},
        {"ownerUserId", room.ownerUserId},
        {"locked", room.locked},
        {"gameStarted", room.gameStarted},
        {"bpEnabled", room.bpEnabled},
        {"players", json::array()},
    };
    for (const auto& [uid, p] : room.players) {
      (void)uid;
      roomJson["players"].push_back({{"userId", p.userId},
                                    {"team", p.team},
                                    {"role", p.role},
                                    {"roleScore", p.roleScore},
                                    {"ready", p.ready},
                                    {"online", p.online},
                                    {"leftEarly", p.leftEarly}});
    }
    root["rooms"].push_back(roomJson);
  }

  root["deletedRooms"] = json::array();
  for (const auto& roomId : deletedRoomIds_) {
    root["deletedRooms"].push_back(roomId);
  }

  root["matches"] = json::array();
  for (const auto& [matchId, m] : matchesById_) {
    (void)matchId;
    root["matches"].push_back({
        {"matchId", m.matchId},
        {"roomId", m.roomId},
        {"ownerUserId", m.ownerUserId},
        {"winner", m.winner},
        {"idempotencyKey", m.idempotencyKey},
        {"createdAtEpochSec", m.createdAtEpochSec},
        {"voteDeadlineEpochSec", m.voteDeadlineEpochSec},
        {"finalized", m.finalized},
        {"passed", m.passed},
        {"snapshotPlayers", m.snapshotPlayers},
        {"approved", json(m.approved)},
        {"rejected", json(m.rejected)},
        {"effectiveVoters", json(m.effectiveVoters)},
    });
  }

  root["matchHistory"] = json::array();
  for (const auto& h : matchHistory_) {
    root["matchHistory"].push_back({
        {"matchId", h.matchId},
        {"roomId", h.roomId},
        {"userId", h.userId},
        {"createdAtEpochSec", h.createdAtEpochSec},
        {"role", h.role},
        {"eloBefore", h.eloBefore},
        {"eloAfter", h.eloAfter},
        {"delta", h.delta},
        {"outcome", h.outcome},
    });
  }

  std::ofstream output(dataFilePath_, std::ios::trunc);
  output << root.dump(2);
  return output.good();
}

}  // namespace simpleelo::server
