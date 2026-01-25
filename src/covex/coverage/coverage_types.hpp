#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace binja::covex::coverage {

enum class TraceFormat { DrcovBlocks, AddrTrace, AddrHitTrace };

struct CoverageSpan {
  uint64_t address = 0;
  uint32_t size = 0;
  uint64_t hits = 0;
  std::optional<uint32_t> module_id;
};

struct ModuleInfo {
  uint32_t id = 0;
  uint64_t base = 0;
  uint64_t end = 0;
  std::string path;
};

struct CoverageTrace {
  TraceFormat format = TraceFormat::DrcovBlocks;
  std::vector<CoverageSpan> spans;
  std::unordered_map<uint32_t, ModuleInfo> modules;
  std::string source_path;
  std::string name;
  bool has_hitcounts = false;
};

} // namespace binja::covex::coverage
