#include "serverEngine.h"

#include <algorithm>

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

double sumWeightedVotes(const std::unordered_set<std::int64_t>& voters,
                        const std::unordered_map<std::int64_t, UserRecord>& usersById) {
  double total = 0.0;
  for (const std::int64_t voterId : voters) {
    const auto userIt = usersById.find(voterId);
    if (userIt == usersById.end()) {
      continue;
    }
    total += simpleelo::elo::voteWeightFromRd(userIt->second.glickoRd);
  }
  return total;
}

}  // namespace

void ServerEngine::finalizeMatchIfNeeded(MatchRecord& match, bool forceDeadlineCheck) {
  if (match.finalized) {
    return;
  }

  const int total = static_cast<int>(match.effectiveVoters.size());
  const int approved = static_cast<int>(match.approved.size());
  const int rejected = static_cast<int>(match.rejected.size());
    const double totalWeight = sumWeightedVotes(match.effectiveVoters, usersById_);
    const double approvedWeight = sumWeightedVotes(match.approved, usersById_);
    const double rejectedWeight = sumWeightedVotes(match.rejected, usersById_);
  const bool deadlineReached = nowEpochSec() >= match.voteDeadlineEpochSec;
    const bool hasMajorityApprove =
      (totalWeight > 0.0) ? (approvedWeight > totalWeight / 2.0) : (approved > total / 2);
    const bool hasMajorityReject =
      (totalWeight > 0.0) ? (rejectedWeight >= totalWeight / 2.0) : (rejected >= (total - total / 2));

  if (!hasMajorityApprove && !hasMajorityReject && !(forceDeadlineCheck && deadlineReached)) {
    return;
  }

  match.finalized = true;
  match.passed = hasMajorityApprove;
  if (!match.passed) {
    return;
  }

  auto roomIt = roomsById_.find(match.roomId);
  if (roomIt == roomsById_.end()) {
    return;
  }

  double redRatingTotal = 0.0;
  double blueRatingTotal = 0.0;
  double redRdTotal = 0.0;
  double blueRdTotal = 0.0;
  double redVolTotal = 0.0;
  double blueVolTotal = 0.0;
  int redCount = 0;
  int blueCount = 0;
  for (const auto& [uid, p] : roomIt->second.players) {
    (void)uid;
    const auto userIt = usersById_.find(p.userId);
    if (userIt == usersById_.end()) {
      continue;
    }
    if (p.team == "blue") {
      blueRatingTotal += userIt->second.glickoRating;
      blueRdTotal += userIt->second.glickoRd;
      blueVolTotal += userIt->second.glickoVolatility;
      ++blueCount;
    } else {
      redRatingTotal += userIt->second.glickoRating;
      redRdTotal += userIt->second.glickoRd;
      redVolTotal += userIt->second.glickoVolatility;
      ++redCount;
    }
  }
  if (redCount == 0 || blueCount == 0) {
    return;
  }

    const simpleelo::elo::Glicko2Rating redAvg{
      redRatingTotal / static_cast<double>(redCount),
      redRdTotal / static_cast<double>(redCount),
      redVolTotal / static_cast<double>(redCount)};
    const simpleelo::elo::Glicko2Rating blueAvg{
      blueRatingTotal / static_cast<double>(blueCount),
      blueRdTotal / static_cast<double>(blueCount),
      blueVolTotal / static_cast<double>(blueCount)};
  const bool redWin = match.winner != "blue";

    const double voteParticipationWeight =
      (totalWeight > 0.0) ? std::clamp((approvedWeight + rejectedWeight) / totalWeight, 0.0, 1.0) : 0.0;
    const double voteConsensus =
      (approvedWeight + rejectedWeight > 0.0) ? (approvedWeight / (approvedWeight + rejectedWeight)) : 0.5;
    const double winnerScore = std::clamp(voteConsensus, 0.5, 1.0);
    const double loserScore = 1.0 - winnerScore;

  for (const auto& [uid, p] : roomIt->second.players) {
    (void)uid;
    auto userIt = usersById_.find(p.userId);
    if (userIt == usersById_.end()) {
      continue;
    }
    const bool onWinner = (redWin && p.team == "red") || (!redWin && p.team == "blue");
    const simpleelo::elo::Glicko2Rating beforeRating{
      userIt->second.glickoRating,
      userIt->second.glickoRd,
      userIt->second.glickoVolatility};
    const simpleelo::elo::Glicko2Rating oppAvg = (p.team == "blue") ? redAvg : blueAvg;
    const auto updated = simpleelo::elo::updateHeadToHeadGlicko2(
      beforeRating,
      oppAvg,
      onWinner ? winnerScore : loserScore,
      voteParticipationWeight);

    const int before = userIt->second.elo;
    userIt->second.glickoRating = updated.firstAfter.rating;
    userIt->second.glickoRd = updated.firstAfter.rd;
    userIt->second.glickoVolatility = updated.firstAfter.volatility;
    userIt->second.elo = simpleelo::elo::ratingToDisplay(updated.firstAfter);
    const int delta = userIt->second.elo - before;
    if (onWinner) {
      userIt->second.wins += 1;
    } else {
      userIt->second.losses += 1;
    }
    matchHistory_.push_back({match.matchId,
                             match.roomId,
                             p.userId,
                             match.createdAtEpochSec,
                             p.role,
                             before,
                             userIt->second.elo,
                             delta,
                             onWinner ? "win" : "lose"});
  }
}

nlohmann::json ServerEngine::buildMatchStatus(const MatchRecord& match) const {
  const int total = static_cast<int>(match.effectiveVoters.size());
  const double approvedWeight = sumWeightedVotes(match.approved, usersById_);
  const double rejectedWeight = sumWeightedVotes(match.rejected, usersById_);
  const double totalWeight = sumWeightedVotes(match.effectiveVoters, usersById_);
  nlohmann::json teamRed = nlohmann::json::array();
  nlohmann::json teamBlue = nlohmann::json::array();
  for (const auto& row : match.snapshotPlayers) {
    const std::string team = row.value("team", "red");
    if (team == "blue") {
      teamBlue.push_back(row);
    } else {
      teamRed.push_back(row);
    }
  }
  return {
      {"matchId", match.matchId},
      {"roomId", match.roomId},
      {"winner", match.winner},
      {"createdAtEpochSec", match.createdAtEpochSec},
      {"approved", static_cast<int>(match.approved.size())},
      {"rejected", static_cast<int>(match.rejected.size())},
      {"totalEffective", total},
      {"approvedWeight", approvedWeight},
      {"rejectedWeight", rejectedWeight},
      {"totalEffectiveWeight", totalWeight},
      {"deadlineEpochSec", match.voteDeadlineEpochSec},
      {"finalized", match.finalized},
      {"passed", match.passed},
      {"teamRed", teamRed},
      {"teamBlue", teamBlue},
  };
}

std::string ServerEngine::handleSubmitMatch(const nlohmann::json& req) {
  const std::string token = req.value("token", "");
  const std::int64_t userId = verifyToken(token);
  if (userId == 0) {
    return jsonErr(451, "invalid token");
  }

  const std::string roomId = req.value("roomId", "");
  const std::string winner = req.value("winner", "red");
  const std::string idempotencyKey = req.value("idempotencyKey", "");
  const protocol::SubmitMatchRequest submitRequest{
      req.value("matchId", ""),
      roomId,
      userId,
      idempotencyKey,
      winner == "blue" ? domain::TeamSide::Blue : domain::TeamSide::Red};
  const auto validation = protocol::api::validateSubmitMatchRequest(submitRequest);
  if (validation.code != 0) {
    return jsonErr(validation.code, validation.message);
  }

  auto roomIt = roomsById_.find(roomId);
  if (roomIt == roomsById_.end()) {
    return jsonErr(453, "room not found");
  }
  if (roomIt->second.ownerUserId != userId) {
    return jsonErr(454, "only owner can submit result");
  }

  if (matchIdByIdempotencyKey_.find(idempotencyKey) != matchIdByIdempotencyKey_.end()) {
    auto existingIt = matchesById_.find(matchIdByIdempotencyKey_[idempotencyKey]);
    if (existingIt != matchesById_.end()) {
      finalizeMatchIfNeeded(existingIt->second, true);
      save();
      return jsonOk({{"idempotent", true}, {"match", buildMatchStatus(existingIt->second)}});
    }
  }

  const std::string matchId = req.value("matchId", "match-" + std::to_string(nowEpochSec()));
  MatchRecord match;
  match.matchId = matchId;
  match.roomId = roomId;
  match.ownerUserId = userId;
  match.winner = winner;
  match.idempotencyKey = idempotencyKey;
  match.createdAtEpochSec = nowEpochSec();
  match.voteDeadlineEpochSec = nowEpochSec() + 60;

  for (const auto& [uid, p] : roomIt->second.players) {
    if (p.online) {
      match.effectiveVoters.insert(uid);
    }

    auto userIt = usersById_.find(uid);
    const std::string nickname = (userIt == usersById_.end()) ? std::string("player")
                                                               : (userIt->second.nickname.empty() ? userIt->second.email
                                                                                                  : userIt->second.nickname);
    match.snapshotPlayers.push_back({
        {"userId", uid},
        {"nickname", nickname},
        {"team", p.team},
        {"role", p.role},
        {"roleScore", p.roleScore},
        {"online", p.online},
        {"leftEarly", p.leftEarly},
    });
  }
  if (match.effectiveVoters.empty()) {
    return jsonErr(455, "no remaining voters in room");
  }

  matchesById_[match.matchId] = match;
  matchIdByIdempotencyKey_[idempotencyKey] = match.matchId;

  auto inserted = matchesById_.find(match.matchId);
  finalizeMatchIfNeeded(inserted->second, false);
  save();

  return jsonOk({{"match", buildMatchStatus(inserted->second)}});
}

std::string ServerEngine::handleVote(const nlohmann::json& req) {
  const std::string token = req.value("token", "");
  const std::int64_t userId = verifyToken(token);
  const std::string matchId = req.value("matchId", "");
  const bool approve = req.value("approve", true);
  const protocol::VoteRequest voteRequest{matchId, userId, approve};
  const auto validation = protocol::api::validateVoteRequest(voteRequest);
  if (validation.code != 0) {
    return jsonErr(validation.code, validation.message);
  }
  auto matchIt = matchesById_.find(matchId);
  if (matchIt == matchesById_.end()) {
    return jsonErr(462, "match not found");
  }

  MatchRecord& match = matchIt->second;
  finalizeMatchIfNeeded(match, true);
  if (match.finalized) {
    save();
    return jsonOk({{"match", buildMatchStatus(match)}});
  }
  if (match.effectiveVoters.find(userId) == match.effectiveVoters.end()) {
    return jsonErr(463, "not an effective voter");
  }

  match.approved.erase(userId);
  match.rejected.erase(userId);
  if (approve) {
    match.approved.insert(userId);
  } else {
    match.rejected.insert(userId);
  }

  finalizeMatchIfNeeded(match, true);
  save();
  return jsonOk({{"match", buildMatchStatus(match)}});
}

std::string ServerEngine::handleGetMatch(const nlohmann::json& req) {
  const std::string matchId = req.value("matchId", "");
  auto matchIt = matchesById_.find(matchId);
  if (matchIt == matchesById_.end()) {
    return jsonErr(471, "match not found");
  }

  finalizeMatchIfNeeded(matchIt->second, true);
  save();
  return jsonOk({{"match", buildMatchStatus(matchIt->second)}});
}

}  // namespace simpleelo::server
