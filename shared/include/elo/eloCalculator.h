#pragma once

#include <cstdint>
#include <utility>

namespace simpleelo::elo {

struct EloConfig {
  int kFactor = 32;
};

struct Glicko2Rating {
  double rating = 1000.0;
  double rd = 350.0;
  double volatility = 0.06;
};

struct Glicko2Config {
  double tau = 0.5;
  double epsilon = 1e-6;
  double ratingOrigin = 1000.0;
  double ratingScale = 173.7178;
  double minRd = 30.0;
  double maxRd = 350.0;
};

struct HeadToHeadUpdate {
  Glicko2Rating firstAfter;
  Glicko2Rating secondAfter;
};

std::pair<int, int> updateTeamElo(int winnerEloBefore, int loserEloBefore, const EloConfig& config);

HeadToHeadUpdate updateHeadToHeadGlicko2(const Glicko2Rating& firstBefore,
                                         const Glicko2Rating& secondBefore,
                                         double firstScore,
                                         double resultWeight,
                                         const Glicko2Config& config = Glicko2Config{});

double voteWeightFromRd(double rd, const Glicko2Config& config = Glicko2Config{});

std::int32_t ratingToDisplay(const Glicko2Rating& rating);

}  // namespace simpleelo::elo
