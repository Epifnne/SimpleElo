#pragma once

#include <utility>

namespace simpleelo::elo {

struct EloConfig {
  int kFactor = 32;
};

std::pair<int, int> updateTeamElo(int winnerEloBefore, int loserEloBefore, const EloConfig& config);

}  // namespace simpleelo::elo
