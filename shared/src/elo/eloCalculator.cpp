#include "elo/eloCalculator.h"

#include <algorithm>
#include <cmath>

namespace simpleelo::elo {
namespace {

double expectedScore(int player, int opponent) {
  const double exponent = static_cast<double>(opponent - player) / 400.0;
  return 1.0 / (1.0 + std::pow(10.0, exponent));
}

double clampDouble(double value, double minValue, double maxValue) {
  return std::max(minValue, std::min(value, maxValue));
}

double gOfPhi(double phi) {
  const double pi = 3.14159265358979323846;
  return 1.0 / std::sqrt(1.0 + (3.0 * phi * phi) / (pi * pi));
}

double expectedFromMu(double mu, double muJ, double phiJ) {
  return 1.0 / (1.0 + std::exp(-gOfPhi(phiJ) * (mu - muJ)));
}

double fValue(double x, double delta, double phi, double v, double a, double tau) {
  const double ex = std::exp(x);
  const double numerator = ex * (delta * delta - phi * phi - v - ex);
  const double denominator = 2.0 * std::pow(phi * phi + v + ex, 2.0);
  return (numerator / denominator) - ((x - a) / (tau * tau));
}

Glicko2Rating updateSinglePlayer(const Glicko2Rating& selfBefore,
                                 const Glicko2Rating& oppBefore,
                                 double score,
                                 double resultWeight,
                                 const Glicko2Config& config) {
  const double boundedWeight = clampDouble(resultWeight, 0.0, 1.0);
  if (boundedWeight <= 0.0) {
    return selfBefore;
  }

  const double mu = (selfBefore.rating - config.ratingOrigin) / config.ratingScale;
  const double phi = selfBefore.rd / config.ratingScale;
  const double sigma = selfBefore.volatility;

  const double muOpp = (oppBefore.rating - config.ratingOrigin) / config.ratingScale;
  const double phiOpp = oppBefore.rd / config.ratingScale;

  const double g = gOfPhi(phiOpp);
  const double e = expectedFromMu(mu, muOpp, phiOpp);

  const double vDenominator = boundedWeight * g * g * e * (1.0 - e);
  if (vDenominator <= 1e-12) {
    return selfBefore;
  }
  const double v = 1.0 / vDenominator;
  const double delta = v * boundedWeight * g * (score - e);

  const double a = std::log(sigma * sigma);
  double A = a;
  double B = 0.0;
  if (delta * delta > (phi * phi + v)) {
    B = std::log(delta * delta - phi * phi - v);
  } else {
    double k = 1.0;
    while (fValue(a - k * config.tau, delta, phi, v, a, config.tau) < 0.0) {
      k += 1.0;
    }
    B = a - k * config.tau;
  }

  double fA = fValue(A, delta, phi, v, a, config.tau);
  double fB = fValue(B, delta, phi, v, a, config.tau);

  while (std::abs(B - A) > config.epsilon) {
    const double C = A + (A - B) * fA / (fB - fA);
    const double fC = fValue(C, delta, phi, v, a, config.tau);
    if (fC * fB < 0.0) {
      A = B;
      fA = fB;
    } else {
      fA /= 2.0;
    }
    B = C;
    fB = fC;
  }

  const double sigmaPrime = std::exp(A / 2.0);
  const double phiStar = std::sqrt(phi * phi + sigmaPrime * sigmaPrime);
  const double phiPrime = 1.0 / std::sqrt((1.0 / (phiStar * phiStar)) + (1.0 / v));
  const double muPrime = mu + phiPrime * phiPrime * boundedWeight * g * (score - e);

  Glicko2Rating after = selfBefore;
  after.rating = config.ratingOrigin + config.ratingScale * muPrime;
  after.rd = clampDouble(config.ratingScale * phiPrime, config.minRd, config.maxRd);
  after.volatility = sigmaPrime;
  return after;
}

}  // namespace

std::pair<int, int> updateTeamElo(int winnerEloBefore, int loserEloBefore, const EloConfig& config) {
  const double winnerExpected = expectedScore(winnerEloBefore, loserEloBefore);
  const double loserExpected = expectedScore(loserEloBefore, winnerEloBefore);

  const int winnerAfter = static_cast<int>(std::lround(winnerEloBefore + config.kFactor * (1.0 - winnerExpected)));
  const int loserAfter = static_cast<int>(std::lround(loserEloBefore + config.kFactor * (0.0 - loserExpected)));
  return {winnerAfter, loserAfter};
}

HeadToHeadUpdate updateHeadToHeadGlicko2(const Glicko2Rating& firstBefore,
                                         const Glicko2Rating& secondBefore,
                                         double firstScore,
                                         double resultWeight,
                                         const Glicko2Config& config) {
  const double boundedScore = clampDouble(firstScore, 0.0, 1.0);
  const Glicko2Rating firstAfter = updateSinglePlayer(firstBefore, secondBefore, boundedScore, resultWeight, config);
  const Glicko2Rating secondAfter = updateSinglePlayer(secondBefore, firstBefore, 1.0 - boundedScore, resultWeight, config);
  return {firstAfter, secondAfter};
}

double voteWeightFromRd(double rd, const Glicko2Config& config) {
  const double boundedRd = clampDouble(rd, config.minRd, config.maxRd);
  const double phi = boundedRd / config.ratingScale;
  return gOfPhi(phi);
}

std::int32_t ratingToDisplay(const Glicko2Rating& rating) {
  return static_cast<std::int32_t>(std::lround(rating.rating));
}

}  // namespace simpleelo::elo
