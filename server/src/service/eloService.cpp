#include <vector>

#include "elo/eloCalculator.h"

namespace simpleelo::server::service {

std::pair<int, int> calculateTeamEloDelta(int redElo, int blueElo, bool redWin) {
  const auto [winnerAfter, loserAfter] = simpleelo::elo::updateTeamElo(
      redWin ? redElo : blueElo,
      redWin ? blueElo : redElo,
      simpleelo::elo::EloConfig{});

  if (redWin) {
    return {winnerAfter, loserAfter};
  }
  return {loserAfter, winnerAfter};
}

}  // namespace simpleelo::server::service
