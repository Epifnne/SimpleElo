#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace simpleelo::domain {

enum class TeamSide {
  Red,
  Blue,
};

enum class MatchStatus {
  Draft,
  Voting,
  Finalized,
  Rejected,
};

struct PlayerSnapshot {
  std::int64_t userId = 0;
  std::string nickname;
  TeamSide side = TeamSide::Red;
  std::string role;
  bool leftEarly = false;
  int eloBefore = 1000;
  int eloAfter = 1000;
};

struct MatchRecord {
  std::string matchId;
  std::string roomId;
  std::int64_t ownerUserId = 0;
  TeamSide winner = TeamSide::Red;
  MatchStatus status = MatchStatus::Draft;
  std::vector<PlayerSnapshot> players;
};

}  // namespace simpleelo::domain
