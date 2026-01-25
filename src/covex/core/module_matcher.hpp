#pragma once

#include <optional>
#include <string>

#include "binaryninjaapi.h"
#include "covex/coverage/coverage_types.hpp"
#include "uitypes.h"

namespace binja::covex::core {

struct ModuleMatch {
  uint32_t id = 0;
  uint64_t module_base = 0;
  uint64_t module_end = 0;
  uint64_t view_image_base = 0;
  int64_t slide = 0;
  bool fallback = false;
  std::string path;
  std::string reason;
};

class ModuleMatcher {
public:
  static std::optional<ModuleMatch> match(const coverage::CoverageTrace &trace,
                                          BinaryViewRef view);
  static std::optional<uint64_t> apply_slide(uint64_t addr, int64_t slide);
};

} // namespace binja::covex::core
