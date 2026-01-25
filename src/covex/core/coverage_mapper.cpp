#include "covex/core/coverage_mapper.hpp"

#include <algorithm>
#include <limits>
#include <unordered_map>
#include <unordered_set>

#include "covex/core/module_matcher.hpp"

namespace binja::covex::core {

namespace {

constexpr uint64_t kMaxInstructionLength = 16;

} // namespace

bool CoverageMapper::is_address_in_view(BinaryViewRef view, uint64_t addr) {
  if (!view) {
    return false;
  }
  return addr >= view->GetStart() && addr < view->GetEnd();
}

uint64_t CoverageMapper::instruction_length(BinaryViewRef view, uint64_t addr,
                                            uint64_t remaining) {
  if (!view) {
    return 1;
  }
  auto arch = view->GetDefaultArchitecture();
  uint64_t length = view->GetInstructionLength(arch, addr);
  if (length == 0 || length > kMaxInstructionLength) {
    length = 1;
  }
  if (remaining > 0 && length > remaining) {
    length = remaining;
  }
  return length;
}

MapResult CoverageMapper::map_trace(const coverage::CoverageTrace &trace,
                                    BinaryViewRef view) {
  coverage::CoverageDataset::HitMap hits;
  std::unordered_set<uint64_t> invalid;
  MapResult result;
  result.diagnostics.spans_total = trace.spans.size();

  const auto match = ModuleMatcher::match(trace, view);
  if (match) {
    result.diagnostics.matched_module_id = match->id;
    result.diagnostics.matched_module_path = match->path;
    result.diagnostics.matched_module_base = match->module_base;
    result.diagnostics.matched_module_end = match->module_end;
    result.diagnostics.view_image_base = match->view_image_base;
    result.diagnostics.match_reason = match->reason;
    result.diagnostics.slide = match->slide;
    result.diagnostics.used_fallback = match->fallback;
  }

  for (const auto &span : trace.spans) {
    if (span.size == 0) {
      continue;
    }
    if (match && span.module_id && *span.module_id != match->id) {
      ++result.diagnostics.spans_skipped;
      continue;
    }

    uint64_t span_address = span.address;
    if (match && span.module_id && *span.module_id == match->id) {
      auto adjusted = ModuleMatcher::apply_slide(span.address, match->slide);
      if (!adjusted) {
        invalid.insert(span.address);
        continue;
      }
      span_address = *adjusted;
    }

    ++result.diagnostics.spans_mapped;
    if (!is_address_in_view(view, span_address)) {
      invalid.insert(span_address);
      continue;
    }
    if (span_address > std::numeric_limits<uint64_t>::max() - span.size) {
      invalid.insert(span_address);
      continue;
    }

    const uint64_t span_end = span_address + span.size;
    uint64_t current = span_address;
    while (current < span_end) {
      if (!is_address_in_view(view, current)) {
        invalid.insert(current);
        break;
      }
      hits[current] += span.hits;
      const uint64_t remaining = span_end - current;
      const uint64_t length = instruction_length(view, current, remaining);
      current += length;
    }
  }

  coverage::CoverageDataset dataset =
      coverage::CoverageDataset::from_hits(std::move(hits));
  auto blocks = derive_blocks_from_hits(dataset.hits(), view);

  result.dataset = std::move(dataset);
  result.blocks = std::move(blocks);
  result.invalid_addresses.assign(invalid.begin(), invalid.end());
  return result;
}

MapResult CoverageMapper::map_dataset(const coverage::CoverageDataset &dataset,
                                      BinaryViewRef view) {
  coverage::CoverageDataset::HitMap hits;
  std::unordered_set<uint64_t> invalid;

  for (const auto &[addr, count] : dataset.hits()) {
    if (!is_address_in_view(view, addr)) {
      invalid.insert(addr);
      continue;
    }
    hits.emplace(addr, count);
  }

  coverage::CoverageDataset mapped =
      coverage::CoverageDataset::from_hits(std::move(hits));
  auto blocks = derive_blocks_from_hits(mapped.hits(), view);

  MapResult result;
  result.dataset = std::move(mapped);
  result.blocks = std::move(blocks);
  result.invalid_addresses.assign(invalid.begin(), invalid.end());
  return result;
}

std::vector<CoveredBlock> CoverageMapper::derive_blocks_from_hits(
    const coverage::CoverageDataset::HitMap &hits, BinaryViewRef view) {
  std::unordered_map<uint64_t, CoveredBlock> blocks;
  for (const auto &[addr, count] : hits) {
    if (!view) {
      break;
    }
    const auto candidates = view->GetBasicBlocksForAddress(addr);
    for (const auto &block : candidates) {
      if (!block) {
        continue;
      }
      const uint64_t start = block->GetStart();
      auto &entry = blocks[start];
      if (entry.start == 0) {
        entry.start = start;
        entry.size = static_cast<uint32_t>(block->GetLength());
      }
      entry.hits += count;
    }
  }

  std::vector<CoveredBlock> result;
  result.reserve(blocks.size());
  for (auto &[addr, block] : blocks) {
    (void)addr;
    result.push_back(block);
  }
  std::sort(result.begin(), result.end(),
            [](const CoveredBlock &a, const CoveredBlock &b) {
              return a.start < b.start;
            });
  return result;
}

} // namespace binja::covex::core
