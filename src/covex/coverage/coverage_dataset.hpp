#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "covex/coverage/coverage_types.hpp"

namespace binja::covex::coverage {

struct CoverageStats {
  size_t total_spans = 0;
  uint64_t total_hits = 0;
  size_t unique_addresses = 0;
};

class CoverageDataset {
public:
  using HitMap = std::unordered_map<uint64_t, uint64_t>;

  CoverageDataset() = default;
  CoverageDataset(HitMap hits, std::vector<CoverageSpan> spans);

  static CoverageDataset from_hits(HitMap hits);

  const HitMap &hits() const { return hits_; }
  const std::vector<CoverageSpan> &spans() const { return spans_; }
  const CoverageStats &stats() const { return stats_; }

private:
  void recompute_stats();

  HitMap hits_;
  std::vector<CoverageSpan> spans_;
  CoverageStats stats_{};
};

} // namespace binja::covex::coverage
