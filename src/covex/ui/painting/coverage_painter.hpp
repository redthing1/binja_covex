#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "covex/coverage/coverage_dataset.hpp"
#include "uitypes.h"

namespace binja::covex::ui {

struct HeatmapSettings {
  uint32_t percentile_cap = 95;
  bool log_scale = true;
  uint8_t alpha = 255;
};

enum class HighlightGranularity { Instruction, BasicBlock };

class CoveragePainter {
public:
  explicit CoveragePainter(BinaryViewRef view);

  void apply_plain(const coverage::CoverageDataset &dataset,
                   HighlightGranularity granularity);
  void apply_heatmap(const coverage::CoverageDataset &dataset,
                     HighlightGranularity granularity,
                     const HeatmapSettings &settings);
  void clear();

private:
  struct InstructionHighlight {
    FunctionRef function;
    uint64_t address = 0;
  };

  struct BlockHighlight {
    BasicBlockRef block;
  };

  using BlockHitMap = std::unordered_map<uint64_t, uint64_t>;

  BlockHitMap
  build_block_hit_map(const coverage::CoverageDataset &dataset) const;
  void apply_plain_instruction(const coverage::CoverageDataset &dataset);
  void apply_plain_block(const coverage::CoverageDataset &dataset);
  void apply_heatmap_instruction(const coverage::CoverageDataset &dataset,
                                 const HeatmapSettings &settings);
  void apply_heatmap_block(const coverage::CoverageDataset &dataset,
                           const HeatmapSettings &settings);

  BinaryViewRef view_;
  std::vector<InstructionHighlight> instruction_highlights_;
  std::vector<BlockHighlight> block_highlights_;
};

} // namespace binja::covex::ui
