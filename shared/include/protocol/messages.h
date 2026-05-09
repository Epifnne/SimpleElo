#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "domain/models.h"

namespace simpleelo::protocol {

struct RegisterRequest {
  std::string email;
  std::string password;
  std::string verifyCode;
};

struct LoginRequest {
  std::string email;
  std::string password;
};

struct ApiResponse {
  int code = 0;
  std::string message = "ok";
};

struct CreateRoomRequest {
  std::string roomName;
  std::int64_t ownerUserId = 0;
};

struct SubmitMatchRequest {
  std::string matchId;
  std::string roomId;
  std::int64_t ownerUserId = 0;
  std::string idempotencyKey;
  domain::TeamSide winner = domain::TeamSide::Red;
};

struct VoteRequest {
  std::string matchId;
  std::int64_t voterUserId = 0;
  bool approve = true;
};

struct VoteSummary {
  std::string matchId;
  int approved = 0;
  int rejected = 0;
  int totalEffective = 0;
  bool passed = false;
};

}  // namespace simpleelo::protocol
