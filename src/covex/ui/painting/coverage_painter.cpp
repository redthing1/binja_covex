#include "covex/ui/painting/coverage_painter.hpp"

#include <algorithm>
#include <cmath>

namespace binja::covex::ui {

namespace {

std::vector<uint64_t>
collect_hitcounts(const coverage::CoverageDataset &dataset) {
  std::vector<uint64_t> counts;
  counts.reserve(dataset.hits().size());
  for (const auto &[addr, count] : dataset.hits()) {
    (void)addr;
    counts.push_back(count);
  }
  return counts;
}

struct HeatmapColor {
  uint8_t red = 0;
  uint8_t green = 0;
  uint8_t blue = 0;
};

HeatmapColor heatmap_color(uint64_t value, uint64_t min_value,
                           uint64_t max_value) {
  if (max_value <= min_value) {
    return {128, 0, 128};
  }
  const double normalized = static_cast<double>(value - min_value) /
                            static_cast<double>(max_value - min_value);
  const auto red = static_cast<uint8_t>(255 * normalized);
  const auto blue = static_cast<uint8_t>(255 * (1.0 - normalized));
  return {red, 0, blue};
}

HeatmapColor heatmap_color_log(uint64_t value, uint64_t min_value,
                               uint64_t max_value) {
  if (max_value <= min_value) {
    return {128, 0, 128};
  }
  const double log_min = std::log(static_cast<double>(min_value) + 1.0);
  const double log_max = std::log(static_cast<double>(max_value) + 1.0);
  const double log_val = std::log(static_cast<double>(value) + 1.0);
  const double normalized = (log_val - log_min) / (log_max - log_min);
  const auto red = static_cast<uint8_t>(255 * std::clamp(normalized, 0.0, 1.0));
  const auto blue =
      static_cast<uint8_t>(255 * (1.0 - std::clamp(normalized, 0.0, 1.0)));
  return {red, 0, blue};
}

uint64_t percentile_cap_value(std::vector<uint64_t> &counts,
                              uint32_t percentile) {
  if (counts.empty()) {
    return 0;
  }
  std::sort(counts.begin(), counts.end());
  const double fraction = static_cast<double>(percentile) / 100.0;
  size_t idx =
      static_cast<size_t>(fraction * static_cast<double>(counts.size() - 1));
  if (idx >= counts.size()) {
    idx = counts.size() - 1;
  }
  return counts[idx];
}

} // namespace

CoveragePainter::CoveragePainter(BinaryViewRef view) : view_(view) {}

void CoveragePainter::apply_plain(const coverage::CoverageDataset &dataset,
                                  HighlightGranularity granularity) {
  clear();
  switch (granularity) {
  case HighlightGranularity::BasicBlock:
    apply_plain_block(dataset);
    break;
  case HighlightGranularity::Instruction:
  default:
    apply_plain_instruction(dataset);
    break;
  }
}

void CoveragePainter::apply_heatmap(const coverage::CoverageDataset &dataset,
                                    HighlightGranularity granularity,
                                    const HeatmapSettings &settings) {
  clear();
  switch (granularity) {
  case HighlightGranularity::BasicBlock:
    apply_heatmap_block(dataset, settings);
    break;
  case HighlightGranularity::Instruction:
  default:
    apply_heatmap_instruction(dataset, settings);
    break;
  }
}

void CoveragePainter::clear() {
  for (const auto &entry : instruction_highlights_) {
    if (!entry.function) {
      continue;
    }
    entry.function->SetAutoInstructionHighlight(
        entry.function->GetArchitecture(), entry.address, NoHighlightColor);
  }
  instruction_highlights_.clear();

  for (const auto &entry : block_highlights_) {
    if (!entry.block) {
      continue;
    }
    entry.block->SetAutoBasicBlockHighlight(NoHighlightColor);
  }
  block_highlights_.clear();
}

CoveragePainter::BlockHitMap CoveragePainter::build_block_hit_map(
    const coverage::CoverageDataset &dataset) const {
  BlockHitMap result;
  if (!view_) {
    return result;
  }
  for (const auto &[addr, count] : dataset.hits()) {
    const auto blocks = view_->GetBasicBlocksForAddress(addr);
    for (const auto &block : blocks) {
      if (!block) {
        continue;
      }
      result[block->GetStart()] += count;
    }
  }
  return result;
}

void CoveragePainter::apply_plain_instruction(
    const coverage::CoverageDataset &dataset) {
  if (!view_) {
    return;
  }
  for (const auto &[addr, count] : dataset.hits()) {
    (void)count;
    const auto funcs = view_->GetAnalysisFunctionsContainingAddress(addr);
    for (const auto &func : funcs) {
      if (!func) {
        continue;
      }
      func->SetAutoInstructionHighlight(func->GetArchitecture(), addr,
                                        OrangeHighlightColor);
      instruction_highlights_.push_back({func, addr});
    }
  }
}

void CoveragePainter::apply_plain_block(
    const coverage::CoverageDataset &dataset) {
  if (!view_) {
    return;
  }
  auto block_hits = build_block_hit_map(dataset);
  for (const auto &[addr, count] : block_hits) {
    (void)count;
    const auto blocks = view_->GetBasicBlocksStartingAtAddress(addr);
    for (const auto &block : blocks) {
      if (!block) {
        continue;
      }
      block->SetAutoBasicBlockHighlight(OrangeHighlightColor);
      block_highlights_.push_back({block});
    }
  }
}

void CoveragePainter::apply_heatmap_instruction(
    const coverage::CoverageDataset &dataset, const HeatmapSettings &settings) {
  if (!view_) {
    return;
  }
  auto counts = collect_hitcounts(dataset);
  if (counts.empty()) {
    return;
  }
  const uint64_t min_value = *std::min_element(counts.begin(), counts.end());
  const uint64_t cap_value =
      percentile_cap_value(counts, settings.percentile_cap);

  for (const auto &[addr, count] : dataset.hits()) {
    const auto funcs = view_->GetAnalysisFunctionsContainingAddress(addr);
    if (funcs.empty()) {
      continue;
    }
    const uint64_t capped = std::min(count, cap_value);
    const HeatmapColor color =
        settings.log_scale ? heatmap_color_log(capped, min_value, cap_value)
                           : heatmap_color(capped, min_value, cap_value);
    for (const auto &func : funcs) {
      if (!func) {
        continue;
      }
      func->SetAutoInstructionHighlight(func->GetArchitecture(), addr,
                                        color.red, color.green, color.blue,
                                        settings.alpha);
      instruction_highlights_.push_back({func, addr});
    }
  }
}

void CoveragePainter::apply_heatmap_block(
    const coverage::CoverageDataset &dataset, const HeatmapSettings &settings) {
  if (!view_) {
    return;
  }
  auto block_hits = build_block_hit_map(dataset);
  if (block_hits.empty()) {
    return;
  }

  std::vector<uint64_t> counts;
  counts.reserve(block_hits.size());
  for (const auto &[addr, count] : block_hits) {
    (void)addr;
    counts.push_back(count);
  }
  const uint64_t min_value = *std::min_element(counts.begin(), counts.end());
  const uint64_t cap_value =
      percentile_cap_value(counts, settings.percentile_cap);

  for (const auto &[addr, count] : block_hits) {
    const auto blocks = view_->GetBasicBlocksStartingAtAddress(addr);
    if (blocks.empty()) {
      continue;
    }
    const uint64_t capped = std::min(count, cap_value);
    const HeatmapColor color =
        settings.log_scale ? heatmap_color_log(capped, min_value, cap_value)
                           : heatmap_color(capped, min_value, cap_value);
    for (const auto &block : blocks) {
      if (!block) {
        continue;
      }
      block->SetAutoBasicBlockHighlight(color.red, color.green, color.blue,
                                        settings.alpha);
      block_highlights_.push_back({block});
    }
  }
}

} // namespace binja::covex::ui
