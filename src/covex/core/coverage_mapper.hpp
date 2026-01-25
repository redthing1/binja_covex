#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "binaryninjaapi.h"
#include "covex/coverage/coverage_dataset.hpp"
#include "covex/coverage/coverage_types.hpp"
#include "uitypes.h"

namespace binja::covex::core {

struct CoveredBlock {
  uint64_t start = 0;
  uint32_t size = 0;
  uint64_t hits = 0;
  std::optional<uint32_t> module_id;
};

struct MapDiagnostics {
  size_t spans_total = 0;
  size_t spans_mapped = 0;
  size_t spans_skipped = 0;
  std::optional<uint32_t> matched_module_id;
  std::optional<uint64_t> matched_module_base;
  std::optional<uint64_t> matched_module_end;
  uint64_t view_image_base = 0;
  std::string matched_module_path;
  std::string match_reason;
  int64_t slide = 0;
  bool used_fallback = false;
};

struct MapResult {
  coverage::CoverageDataset dataset;
  std::vector<CoveredBlock> blocks;
  std::vector<uint64_t> invalid_addresses;
  MapDiagnostics diagnostics;
};

class CoverageMapper {
public:
  MapResult map_trace(const coverage::CoverageTrace &trace, BinaryViewRef view);
  MapResult map_dataset(const coverage::CoverageDataset &dataset,
                        BinaryViewRef view);

private:
  static std::vector<CoveredBlock>
  derive_blocks_from_hits(const coverage::CoverageDataset::HitMap &hits,
                          BinaryViewRef view);
  static bool is_address_in_view(BinaryViewRef view, uint64_t addr);
  static bool has_function(BinaryViewRef view, uint64_t addr);
  static uint64_t instruction_length(BinaryViewRef view, uint64_t addr,
                                     uint64_t remaining);
};

} // namespace binja::covex::core
