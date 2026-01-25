#include "covex/coverage/coverage_operations.hpp"

#include <algorithm>

namespace binja::covex::coverage {

namespace {

uint64_t merge_hits(uint64_t left, uint64_t right, HitMergePolicy policy) {
  switch (policy) {
  case HitMergePolicy::Sum:
    return left + right;
  case HitMergePolicy::Min:
    return std::min(left, right);
  case HitMergePolicy::Max:
    return std::max(left, right);
  case HitMergePolicy::Left:
  default:
    return left;
  }
}

} // namespace

CoverageDataset compose(const CoverageDataset &a, const CoverageDataset &b,
                        CompositionOp op, HitMergePolicy policy) {
  CoverageDataset::HitMap result;

  const auto &hits_a = a.hits();
  const auto &hits_b = b.hits();

  switch (op) {
  case CompositionOp::Union: {
    result = hits_a;
    for (const auto &[addr, count] : hits_b) {
      auto it = result.find(addr);
      if (it == result.end()) {
        result.emplace(addr, count);
      } else {
        it->second = merge_hits(it->second, count, policy);
      }
    }
    break;
  }
  case CompositionOp::Intersection: {
    for (const auto &[addr, count] : hits_a) {
      auto it = hits_b.find(addr);
      if (it == hits_b.end()) {
        continue;
      }
      result.emplace(addr, merge_hits(count, it->second, policy));
    }
    break;
  }
  case CompositionOp::Subtract: {
    for (const auto &[addr, count] : hits_a) {
      if (hits_b.find(addr) != hits_b.end()) {
        continue;
      }
      result.emplace(addr, count);
    }
    break;
  }
  default:
    break;
  }

  return CoverageDataset::from_hits(std::move(result));
}

} // namespace binja::covex::coverage
