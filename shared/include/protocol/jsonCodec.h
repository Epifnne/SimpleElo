#pragma once

#include <string>

#include "domain/models.h"
#include "protocol/messages.h"

namespace simpleelo::protocol {

std::string encodeSubmitMatchRequest(const SubmitMatchRequest& request);
SubmitMatchRequest decodeSubmitMatchRequest(const std::string& payload);

std::string encodeVoteSummary(const VoteSummary& summary);
VoteSummary decodeVoteSummary(const std::string& payload);

}  // namespace simpleelo::protocol
