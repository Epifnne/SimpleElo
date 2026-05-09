#include "protocol/jsonCodec.h"

#include <stdexcept>

#include <nlohmann/json.hpp>

namespace simpleelo::protocol {
namespace {

using nlohmann::json;

std::string toString(domain::TeamSide side) {
  return side == domain::TeamSide::Red ? "red" : "blue";
}

domain::TeamSide toTeamSide(const std::string& value) {
  if (value == "blue") {
    return domain::TeamSide::Blue;
  }
  return domain::TeamSide::Red;
}

}  // namespace

std::string encodeSubmitMatchRequest(const SubmitMatchRequest& request) {
  json payload = {
      {"matchId", request.matchId},
      {"roomId", request.roomId},
      {"ownerUserId", request.ownerUserId},
      {"idempotencyKey", request.idempotencyKey},
      {"winner", toString(request.winner)},
  };
  return payload.dump();
}

SubmitMatchRequest decodeSubmitMatchRequest(const std::string& payload) {
  const json data = json::parse(payload);
  SubmitMatchRequest request;
  request.matchId = data.at("matchId").get<std::string>();
  request.roomId = data.at("roomId").get<std::string>();
  request.ownerUserId = data.at("ownerUserId").get<std::int64_t>();
  request.idempotencyKey = data.at("idempotencyKey").get<std::string>();
  request.winner = toTeamSide(data.value("winner", "red"));
  return request;
}

std::string encodeVoteSummary(const VoteSummary& summary) {
  json payload = {
      {"matchId", summary.matchId},
      {"approved", summary.approved},
      {"rejected", summary.rejected},
      {"totalEffective", summary.totalEffective},
      {"passed", summary.passed},
  };
  return payload.dump();
}

VoteSummary decodeVoteSummary(const std::string& payload) {
  const json data = json::parse(payload);
  VoteSummary summary;
  summary.matchId = data.at("matchId").get<std::string>();
  summary.approved = data.at("approved").get<int>();
  summary.rejected = data.at("rejected").get<int>();
  summary.totalEffective = data.at("totalEffective").get<int>();
  summary.passed = data.at("passed").get<bool>();
  return summary;
}

}  // namespace simpleelo::protocol
