#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "binaryninjaapi.h"
#include "covex/core/coverage_index.hpp"
#include "covex/coverage/coverage_dataset.hpp"
#include "covex/coverage/coverage_types.hpp"
#include "uitypes.h"

namespace binja::covex::core {

class CoverageMapper {
public:
  CoverageIndex map_trace(const coverage::CoverageTrace &trace,
                          BinaryViewRef view);
  CoverageIndex map_dataset(const coverage::CoverageDataset &dataset,
                            BinaryViewRef view);

private:
  static std::vector<CoveredBlock>
  derive_blocks_from_hits(const coverage::CoverageDataset::HitMap &hits,
                          BinaryViewRef view);
  static bool is_address_in_view(BinaryViewRef view, uint64_t addr);
  static uint64_t instruction_length(BinaryViewRef view, uint64_t addr,
                                     uint64_t remaining);
};

} // namespace binja::covex::core
