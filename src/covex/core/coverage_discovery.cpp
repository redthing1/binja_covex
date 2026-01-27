#include "covex/core/coverage_discovery.hpp"

#include <unordered_set>

#include "binaryninjaapi.h"

namespace binja::covex::core {

namespace {

uint64_t max_instruction_length(BinaryViewRef view) {
  if (!view) {
    return 16;
  }
  auto arch = view->GetDefaultArchitecture();
  if (!arch) {
    return 16;
  }
  const auto max_len = arch->GetMaxInstructionLength();
  return max_len == 0 ? 16 : static_cast<uint64_t>(max_len);
}

bool is_valid_instruction(BinaryViewRef view, uint64_t addr, uint64_t max_len) {
  if (!view) {
    return false;
  }
  auto arch = view->GetDefaultArchitecture();
  if (!arch) {
    return false;
  }
  const uint64_t length = view->GetInstructionLength(arch, addr);
  return length != 0 && length <= max_len;
}

bool is_executable_segment(BinaryViewRef view, uint64_t addr,
                           const CoverageDiscoverySettings &settings,
                           DiscoverySkipReason &reason) {
  if (!view) {
    reason = DiscoverySkipReason::NonExecutableSegment;
    return false;
  }
  auto segment = view->GetSegmentAt(addr);
  if (!segment) {
    reason = DiscoverySkipReason::NonExecutableSegment;
    return false;
  }
  const uint32_t flags = segment->GetFlags();
  if ((flags & SegmentDenyExecute) != 0) {
    reason = DiscoverySkipReason::DenyExecute;
    return false;
  }
  if ((flags & SegmentExecutable) == 0) {
    reason = DiscoverySkipReason::NonExecutableSegment;
    return false;
  }
  if (settings.require_segment_code_flag &&
      (flags & SegmentContainsCode) == 0) {
    reason = DiscoverySkipReason::NonExecutableSegment;
    return false;
  }
  return true;
}

bool is_code_section(BinaryViewRef view, uint64_t addr,
                     DiscoverySkipReason &reason) {
  if (!view) {
    reason = DiscoverySkipReason::SectionNotCode;
    return false;
  }
  const auto sections = view->GetSectionsAt(addr);
  if (sections.empty()) {
    reason = DiscoverySkipReason::SectionNotCode;
    return false;
  }
  for (const auto &section : sections) {
    if (!section) {
      continue;
    }
    const auto semantics = section->GetSemantics();
    if (semantics == ReadOnlyCodeSectionSemantics) {
      return true;
    }
  }
  reason = DiscoverySkipReason::SectionNotCode;
  return false;
}

bool has_data_variable(BinaryViewRef view, uint64_t addr) {
  if (!view) {
    return false;
  }
  BinaryNinja::DataVariable var;
  return view->GetDataVariableAtAddress(addr, var);
}

uint64_t find_entrypoint_linear(BinaryViewRef view, uint64_t addr,
                                uint64_t backward_scan_bytes) {
  if (!view || backward_scan_bytes == 0) {
    return addr;
  }
  auto arch = view->GetDefaultArchitecture();
  if (!arch) {
    return addr;
  }
  const uint64_t max_len = max_instruction_length(view);
  size_t alignment = arch->GetInstructionAlignment();
  if (alignment == 0) {
    alignment = 1;
  }
  uint64_t start = addr > backward_scan_bytes ? addr - backward_scan_bytes
                                              : view->GetStart();
  if (start < view->GetStart()) {
    start = view->GetStart();
  }
  uint64_t best = addr;
  uint64_t candidate = addr;
  while (true) {
    if (view->IsValidOffset(candidate) &&
        is_valid_instruction(view, candidate, max_len)) {
      uint64_t cursor = candidate;
      bool ok = true;
      while (cursor < addr) {
        const uint64_t length = view->GetInstructionLength(arch, cursor);
        if (length == 0 || length > max_len) {
          ok = false;
          break;
        }
        if (cursor + length > addr) {
          ok = false;
          break;
        }
        cursor += length;
      }
      if (ok && cursor == addr) {
        best = candidate;
      }
    }
    if (candidate <= start) {
      break;
    }
    uint64_t next = candidate - alignment;
    if (next > candidate) {
      break;
    }
    candidate = next;
  }
  return best;
}

} // namespace

const char *discovery_skip_reason_label(DiscoverySkipReason reason) {
  switch (reason) {
  case DiscoverySkipReason::InFunction:
    return "in-function";
  case DiscoverySkipReason::NonExecutableSegment:
    return "non-exec-segment";
  case DiscoverySkipReason::DenyExecute:
    return "deny-exec";
  case DiscoverySkipReason::DataVariable:
    return "data-variable";
  case DiscoverySkipReason::InvalidInstruction:
    return "invalid-instruction";
  case DiscoverySkipReason::SectionNotCode:
    return "section-not-code";
  case DiscoverySkipReason::InvalidAddress:
    return "invalid-address";
  case DiscoverySkipReason::CreateFailed:
    return "create-failed";
  }
  return "unknown";
}

CoverageDiscoveryPlan
BuildDiscoveryPlan(const CoverageIndex &index, BinaryViewRef view,
                   const CoverageDiscoverySettings &settings) {
  CoverageDiscoveryPlan plan;
  if (!view) {
    return plan;
  }
  const uint64_t max_len = max_instruction_length(view);
  for (const auto addr : index.hit_addresses_sorted) {
    CoverageDiscoveryCandidate candidate;
    candidate.hit_address = addr;
    candidate.entrypoint = addr;

    if (!view->IsValidOffset(addr)) {
      candidate.skip_reason = DiscoverySkipReason::InvalidAddress;
      plan.candidates.push_back(candidate);
      continue;
    }
    if (!view->GetAnalysisFunctionsContainingAddress(addr).empty()) {
      candidate.skip_reason = DiscoverySkipReason::InFunction;
      plan.candidates.push_back(candidate);
      continue;
    }
    DiscoverySkipReason reason = DiscoverySkipReason::NonExecutableSegment;
    if (!is_executable_segment(view, addr, settings, reason)) {
      candidate.skip_reason = reason;
      plan.candidates.push_back(candidate);
      continue;
    }
    if (settings.require_code_section) {
      if (!is_code_section(view, addr, reason)) {
        candidate.skip_reason = reason;
        plan.candidates.push_back(candidate);
        continue;
      }
    }
    if (has_data_variable(view, addr)) {
      candidate.skip_reason = DiscoverySkipReason::DataVariable;
      plan.candidates.push_back(candidate);
      continue;
    }
    if (!is_valid_instruction(view, addr, max_len)) {
      candidate.skip_reason = DiscoverySkipReason::InvalidInstruction;
      plan.candidates.push_back(candidate);
      continue;
    }

    candidate.entrypoint =
        find_entrypoint_linear(view, addr, settings.backward_scan_bytes);
    plan.candidates.push_back(candidate);
  }
  return plan;
}

DiscoveryReport
ExecuteDiscoveryPlan(const CoverageDiscoveryPlan &plan, BinaryViewRef view,
                     const CoverageDiscoverySettings &settings) {
  DiscoveryReport report;
  if (!view) {
    return report;
  }
  auto platform = view->GetDefaultPlatform();
  if (!platform) {
    return report;
  }

  std::unordered_set<uint64_t> seen_entrypoints;
  bool created_any = false;

  for (const auto &candidate : plan.candidates) {
    report.candidates += 1;
    if (candidate.skip_reason) {
      report.skipped[*candidate.skip_reason] += 1;
      continue;
    }

    if (!view->GetAnalysisFunctionsContainingAddress(candidate.hit_address)
             .empty()) {
      report.skipped[DiscoverySkipReason::InFunction] += 1;
      continue;
    }
    if (seen_entrypoints.count(candidate.entrypoint) != 0) {
      continue;
    }
    if (!view->GetAnalysisFunctionsForAddress(candidate.entrypoint).empty()) {
      report.skipped[DiscoverySkipReason::InFunction] += 1;
      continue;
    }
    if (!view->IsValidOffset(candidate.entrypoint)) {
      report.skipped[DiscoverySkipReason::InvalidAddress] += 1;
      continue;
    }

    auto func = view->CreateUserFunction(platform, candidate.entrypoint);
    if (!func) {
      report.skipped[DiscoverySkipReason::CreateFailed] += 1;
      continue;
    }
    seen_entrypoints.insert(candidate.entrypoint);
    report.created += 1;
    created_any = true;

    if (settings.update_analysis_per_function) {
      view->UpdateAnalysisAndWait();
    }
  }

  if (created_any && !settings.update_analysis_per_function) {
    view->UpdateAnalysisAndWait();
  }

  return report;
}

} // namespace binja::covex::core
