#pragma once

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

#include "binaryninjaapi.h"
#include "covex/core/coverage_index.hpp"
#include "uitypes.h"

namespace binja::covex::core {

enum class DiscoverySkipReason {
  InFunction,
  NonExecutableSegment,
  DenyExecute,
  DataVariable,
  InvalidInstruction,
  SectionNotCode,
  InvalidAddress,
  CreateFailed
};

const char *discovery_skip_reason_label(DiscoverySkipReason reason);

struct CoverageDiscoverySettings {
  uint64_t backward_scan_bytes = 0;
  bool require_code_section = false;
  bool require_segment_code_flag = false;
  bool update_analysis_per_function = true;
};

struct CoverageDiscoveryCandidate {
  uint64_t hit_address = 0;
  uint64_t entrypoint = 0;
  std::optional<DiscoverySkipReason> skip_reason;
};

struct CoverageDiscoveryPlan {
  std::vector<CoverageDiscoveryCandidate> candidates;
};

struct DiscoveryReport {
  size_t candidates = 0;
  size_t created = 0;
  std::unordered_map<DiscoverySkipReason, size_t> skipped;
};

CoverageDiscoveryPlan
BuildDiscoveryPlan(const CoverageIndex &index, BinaryViewRef view,
                   const CoverageDiscoverySettings &settings);

DiscoveryReport ExecuteDiscoveryPlan(const CoverageDiscoveryPlan &plan,
                                     BinaryViewRef view,
                                     const CoverageDiscoverySettings &settings);

} // namespace binja::covex::core
