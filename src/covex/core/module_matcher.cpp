#include "covex/core/module_matcher.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <limits>

namespace binja::covex::core {

namespace {

std::string lower_copy(const std::string &value) {
  std::string out = value;
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return out;
}

std::string base_name(const std::string &path) {
  if (path.empty()) {
    return {};
  }
  return std::filesystem::path(path).filename().string();
}

int64_t compute_slide(uint64_t view_image_base, uint64_t module_base) {
  return static_cast<int64_t>(view_image_base) -
         static_cast<int64_t>(module_base);
}

} // namespace

std::optional<ModuleMatch>
ModuleMatcher::match(const coverage::CoverageTrace &trace, BinaryViewRef view) {
  if (!view || trace.modules.empty()) {
    return std::nullopt;
  }
  auto file = view->GetFile();
  if (!file) {
    return std::nullopt;
  }
  const std::string view_path = file->GetOriginalFilename();
  if (view_path.empty()) {
    return std::nullopt;
  }
  const std::string view_path_lower = lower_copy(view_path);
  const std::string view_base_lower = lower_copy(base_name(view_path));
  const uint64_t view_image_base = view->GetImageBase();

  auto make_match = [&](const coverage::ModuleInfo &module,
                        const std::string &reason, bool fallback) {
    ModuleMatch match;
    match.id = module.id;
    match.module_base = module.base;
    match.module_end = module.end;
    match.view_image_base = view_image_base;
    match.slide = compute_slide(view_image_base, module.base);
    match.fallback = fallback;
    match.path = module.path;
    match.reason = reason;
    return match;
  };

  for (const auto &[id, module] : trace.modules) {
    (void)id;
    if (module.path.empty()) {
      continue;
    }
    const std::string module_path_lower = lower_copy(module.path);
    if (module_path_lower == view_path_lower) {
      return make_match(module, "path", false);
    }
  }

  std::optional<ModuleMatch> best_base;
  uint64_t best_abs_slide = 0;
  bool have_best = false;
  for (const auto &[id, module] : trace.modules) {
    (void)id;
    if (module.path.empty()) {
      continue;
    }
    const std::string module_base_lower = lower_copy(base_name(module.path));
    if (module_base_lower != view_base_lower) {
      continue;
    }
    const uint64_t abs_slide = static_cast<uint64_t>(
        std::llabs(compute_slide(view_image_base, module.base)));
    if (!have_best || abs_slide < best_abs_slide) {
      best_base = make_match(module, "basename", false);
      best_abs_slide = abs_slide;
      have_best = true;
    }
  }
  if (best_base) {
    return best_base;
  }

  if (trace.modules.size() == 1) {
    const auto &module = trace.modules.begin()->second;
    return make_match(module, "single-module", true);
  }

  return std::nullopt;
}

std::optional<uint64_t> ModuleMatcher::apply_slide(uint64_t addr,
                                                   int64_t slide) {
  if (slide == 0) {
    return addr;
  }
  if (slide < 0) {
    const uint64_t delta = static_cast<uint64_t>(-slide);
    if (addr < delta) {
      return std::nullopt;
    }
    return addr - delta;
  }
  const uint64_t delta = static_cast<uint64_t>(slide);
  if (addr > std::numeric_limits<uint64_t>::max() - delta) {
    return std::nullopt;
  }
  return addr + delta;
}

} // namespace binja::covex::core
