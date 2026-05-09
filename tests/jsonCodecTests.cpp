#include <gtest/gtest.h>

#include "protocol/jsonCodec.h"

TEST(JsonCodecTests, SubmitRequestRoundtrip) {
  simpleelo::protocol::SubmitMatchRequest request;
  request.matchId = "m-1001";
  request.roomId = "r-01";
  request.ownerUserId = 42;
  request.idempotencyKey = "idem-1";
  request.winner = simpleelo::domain::TeamSide::Blue;

  const std::string encoded = simpleelo::protocol::encodeSubmitMatchRequest(request);
  const auto decoded = simpleelo::protocol::decodeSubmitMatchRequest(encoded);

  EXPECT_EQ(decoded.matchId, request.matchId);
  EXPECT_EQ(decoded.roomId, request.roomId);
  EXPECT_EQ(decoded.ownerUserId, request.ownerUserId);
  EXPECT_EQ(decoded.idempotencyKey, request.idempotencyKey);
  EXPECT_EQ(decoded.winner, request.winner);
}

TEST(JsonCodecTests, VoteSummaryRoundtrip) {
  simpleelo::protocol::VoteSummary summary;
  summary.matchId = "m-2001";
  summary.approved = 6;
  summary.rejected = 4;
  summary.totalEffective = 10;
  summary.passed = true;

  const std::string encoded = simpleelo::protocol::encodeVoteSummary(summary);
  const auto decoded = simpleelo::protocol::decodeVoteSummary(encoded);

  EXPECT_EQ(decoded.matchId, summary.matchId);
  EXPECT_EQ(decoded.approved, summary.approved);
  EXPECT_EQ(decoded.rejected, summary.rejected);
  EXPECT_EQ(decoded.totalEffective, summary.totalEffective);
  EXPECT_EQ(decoded.passed, summary.passed);
}
