#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "covex/coverage/coverage_dataset.hpp"
#include "covex/coverage/coverage_types.hpp"

namespace binja::covex::core {

struct CoveredBlock {
  uint64_t start = 0;
  uint32_t size = 0;
  uint64_t hits = 0;
  std::optional<uint32_t> module_id;
  std::string function;
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

struct CoverageIndex {
  coverage::CoverageDataset dataset;
  std::vector<CoveredBlock> blocks;
  std::vector<uint64_t> hit_addresses_sorted;
  std::vector<uint64_t> invalid_addresses;
  MapDiagnostics diagnostics;
};

} // namespace binja::covex::core
