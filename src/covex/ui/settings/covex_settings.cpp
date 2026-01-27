#include "covex/ui/settings/covex_settings.hpp"

namespace {

constexpr const char *kSettingsGroup = "covex";
constexpr const char *kDefaultHighlightColorKey = "covex.defaultHighlightColor";
constexpr const char *kHeatmapPercentileKey = "covex.heatmapPercentileCap";
constexpr const char *kHeatmapLogScaleKey = "covex.heatmapLogScale";
constexpr const char *kHighlightAlphaKey = "covex.highlightAlpha";
constexpr const char *kHighlightGranularityKey = "covex.highlightGranularity";
constexpr const char *kDiscoveryRequireCodeSectionKey =
    "covex.discovery.requireCodeSection";
constexpr const char *kDiscoveryBackscanBytesKey =
    "covex.discovery.backwardScanBytes";
constexpr const char *kDiscoveryUpdateAnalysisKey =
    "covex.discovery.updateAnalysisPerFunction";
constexpr const char *kDiscoveryRequireSegmentCodeFlagKey =
    "covex.discovery.requireSegmentCodeFlag";

} // namespace

namespace binja::covex::ui {

void register_settings() {
  auto settings = BinaryNinja::Settings::Instance();
  settings->RegisterGroup(kSettingsGroup, "CovEx");
  settings->RegisterSetting(kDefaultHighlightColorKey,
                            R"json({
      "title" : "Default Highlight Color",
      "type" : "string",
      "default" : "orange",
      "description" : "Default highlight color for coverage overlays.",
      "enum" : ["orange", "cyan", "red", "blue", "green", "magenta", "yellow"],
      "enumDescriptions" : ["orange (default)", "cyan", "red", "blue", "green", "magenta", "yellow"]
    })json");
  settings->RegisterSetting(kHeatmapPercentileKey,
                            R"json({
      "title" : "Heatmap Percentile Cap",
      "type" : "number",
      "default" : 95,
      "description" : "Percentile to cap hitcounts for heatmap coloring.",
      "min" : 50,
      "max" : 100
    })json");
  settings->RegisterSetting(kHeatmapLogScaleKey,
                            R"json({
      "title" : "Heatmap Log Scale",
      "type" : "boolean",
      "default" : true,
      "description" : "Use logarithmic scaling for heatmap coloring."
    })json");
  settings->RegisterSetting(kHighlightAlphaKey,
                            R"json({
      "title" : "Highlight Alpha",
      "type" : "number",
      "default" : 255,
      "description" : "Alpha value for coverage highlights.",
      "min" : 16,
      "max" : 255
    })json");
  settings->RegisterSetting(kHighlightGranularityKey,
                            R"json({
      "title" : "Highlight Granularity",
      "type" : "string",
      "default" : "instruction",
      "description" : "Default highlight granularity.",
      "enum" : ["instruction", "basic_block"],
      "enumDescriptions" : ["Instruction", "Basic Block"]
    })json");
  settings->RegisterSetting(kDiscoveryRequireCodeSectionKey,
                            R"json({
      "title" : "Discovery Require Code Section",
      "type" : "boolean",
      "default" : false,
      "description" : "Require ReadOnlyCode section semantics when defining functions from coverage."
    })json");
  settings->RegisterSetting(kDiscoveryBackscanBytesKey,
                            R"json({
      "title" : "Discovery Backward Scan Bytes",
      "type" : "number",
      "default" : 0,
      "description" : "Maximum bytes to scan backwards when choosing coverage-based function entrypoints.",
      "min" : 0,
      "max" : 256
    })json");
  settings->RegisterSetting(kDiscoveryUpdateAnalysisKey,
                            R"json({
      "title" : "Discovery Update Analysis Per Function",
      "type" : "boolean",
      "default" : true,
      "description" : "Run UpdateAnalysisAndWait after each function definition."
    })json");
  settings->RegisterSetting(kDiscoveryRequireSegmentCodeFlagKey,
                            R"json({
      "title" : "Discovery Require Segment Code Flag",
      "type" : "boolean",
      "default" : false,
      "description" : "Require SegmentContainsCode in addition to SegmentExecutable."
    })json");
}

} // namespace binja::covex::ui
