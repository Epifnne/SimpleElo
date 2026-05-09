#include "protocol/messages.h"

namespace simpleelo::server::service {

protocol::VoteSummary evaluateVote(const protocol::VoteSummary& current) {
  protocol::VoteSummary next = current;
  next.passed = next.approved > (next.totalEffective / 2);
  return next;
}

}  // namespace simpleelo::server::service
