#include <gtest/gtest.h>

#include "elo/eloCalculator.h"

TEST(Glicko2Tests, WinnerGainsLoserDrops) {
  const simpleelo::elo::Glicko2Rating red{1000.0, 120.0, 0.06};
  const simpleelo::elo::Glicko2Rating blue{1000.0, 120.0, 0.06};

  const auto updated = simpleelo::elo::updateHeadToHeadGlicko2(red, blue, 1.0, 1.0);

  EXPECT_GT(updated.firstAfter.rating, red.rating);
  EXPECT_LT(updated.secondAfter.rating, blue.rating);
}

TEST(Glicko2Tests, ConsensusWeightScalesDelta) {
  const simpleelo::elo::Glicko2Rating first{1000.0, 90.0, 0.06};
  const simpleelo::elo::Glicko2Rating second{1000.0, 90.0, 0.06};

  const auto strong = simpleelo::elo::updateHeadToHeadGlicko2(first, second, 0.95, 1.0);
  const auto weak = simpleelo::elo::updateHeadToHeadGlicko2(first, second, 0.95, 0.35);

  const double strongDelta = strong.firstAfter.rating - first.rating;
  const double weakDelta = weak.firstAfter.rating - first.rating;
  EXPECT_GT(strongDelta, weakDelta);
}

TEST(Glicko2Tests, LowerRdCarriesHigherVoteWeight) {
  const double lowRdWeight = simpleelo::elo::voteWeightFromRd(60.0);
  const double highRdWeight = simpleelo::elo::voteWeightFromRd(300.0);

  EXPECT_GT(lowRdWeight, highRdWeight);
}
