#include "covex/ui/settings/covex_settings.hpp"

namespace {

constexpr const char *kSettingsGroup = "covex";
constexpr const char *kDefaultHighlightColorKey = "covex.defaultHighlightColor";
constexpr const char *kHeatmapPercentileKey = "covex.heatmapPercentileCap";
constexpr const char *kHeatmapLogScaleKey = "covex.heatmapLogScale";
constexpr const char *kHighlightAlphaKey = "covex.highlightAlpha";
constexpr const char *kHighlightGranularityKey = "covex.highlightGranularity";

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
}

} // namespace binja::covex::ui
