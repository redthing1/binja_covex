#pragma once

#include "binaryninjaapi.h"
#include "uitypes.h"

namespace binja::covex::log {

inline constexpr const char *kLogger = "CovEx";

inline BinaryNinja::Ref<BinaryNinja::Logger> logger(BinaryViewRef view,
                                                    const char *name) {
  if (view) {
    return view->CreateLogger(name);
  }
  return BinaryNinja::LogRegistry::CreateLogger(name);
}

inline BinaryNinja::Ref<BinaryNinja::Logger>
logger(BinaryNinja::BinaryView *view, const char *name) {
  if (view) {
    return view->CreateLogger(name);
  }
  return BinaryNinja::LogRegistry::CreateLogger(name);
}

} // namespace binja::covex::log
