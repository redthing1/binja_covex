#include "covex/coverage/coverage_dataset.hpp"

namespace binja::covex::coverage {

CoverageDataset::CoverageDataset(HitMap hits, std::vector<CoverageSpan> spans)
    : hits_(std::move(hits)), spans_(std::move(spans)) {
  recompute_stats();
}

CoverageDataset CoverageDataset::from_hits(HitMap hits) {
  std::vector<CoverageSpan> spans;
  spans.reserve(hits.size());
  for (const auto &[addr, count] : hits) {
    CoverageSpan span;
    span.address = addr;
    span.size = 1;
    span.hits = count;
    spans.push_back(span);
  }
  return CoverageDataset(std::move(hits), std::move(spans));
}

void CoverageDataset::recompute_stats() {
  stats_.total_spans = spans_.size();
  stats_.unique_addresses = hits_.size();
  stats_.total_hits = 0;
  for (const auto &[addr, count] : hits_) {
    (void)addr;
    stats_.total_hits += count;
  }
}

} // namespace binja::covex::coverage
