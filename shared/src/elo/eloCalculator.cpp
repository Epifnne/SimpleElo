#include "elo/eloCalculator.h"

#include <cmath>

namespace simpleelo::elo {
namespace {

double expectedScore(int player, int opponent) {
  const double exponent = static_cast<double>(opponent - player) / 400.0;
  return 1.0 / (1.0 + std::pow(10.0, exponent));
}

}  // namespace

std::pair<int, int> updateTeamElo(int winnerEloBefore, int loserEloBefore, const EloConfig& config) {
  const double winnerExpected = expectedScore(winnerEloBefore, loserEloBefore);
  const double loserExpected = expectedScore(loserEloBefore, winnerEloBefore);

  const int winnerAfter = static_cast<int>(std::lround(winnerEloBefore + config.kFactor * (1.0 - winnerExpected)));
  const int loserAfter = static_cast<int>(std::lround(loserEloBefore + config.kFactor * (0.0 - loserExpected)));
  return {winnerAfter, loserAfter};
}

}  // namespace simpleelo::elo
