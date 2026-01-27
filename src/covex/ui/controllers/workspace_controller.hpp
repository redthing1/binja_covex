#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "binaryninjaapi.h"
#include "covex/core/block_filter.hpp"
#include "covex/core/coverage_index.hpp"
#include "covex/core/coverage_mapper.hpp"
#include "covex/coverage/addr_trace_reader.hpp"
#include "covex/coverage/coverage_expression.hpp"
#include "covex/coverage/coverage_parser.hpp"
#include "covex/coverage/drcov_reader.hpp"
#include "covex/ui/painting/coverage_painter.hpp"
#include "uitypes.h"

namespace binja::covex::ui {

struct TraceSummary {
  std::string alias;
  std::string name;
  uint64_t spans = 0;
  uint64_t unique_addresses = 0;
  uint64_t total_hits = 0;
  bool has_hitcounts = false;
};

struct BlockSummary {
  uint64_t address = 0;
  uint32_t size = 0;
  uint64_t hits = 0;
  std::string function;
};

class CoverageWorkspaceView {
public:
  virtual ~CoverageWorkspaceView() = default;
  virtual void set_traces(const std::vector<TraceSummary> &traces) = 0;
  virtual void set_blocks(const std::vector<BlockSummary> &blocks) = 0;
  virtual void show_expression_error(const std::string &message) = 0;
  virtual void clear_expression_error() = 0;
};

enum class HighlightMode { Plain, Heatmap };

class CoverageWorkspaceController final {
public:
  CoverageWorkspaceController(BinaryViewRef view,
                              CoverageWorkspaceView &view_ui);
  ~CoverageWorkspaceController();

  BinaryViewRef view() const { return view_; }

  bool prompt_load();
  bool load_trace_file(const std::string &path);
  void clear_highlights();
  void set_expression(const std::string &expression);
  void set_block_filter(const std::string &filter_text);
  void set_highlight_mode(HighlightMode mode);
  void set_granularity(HighlightGranularity granularity);
  void set_heatmap_settings(const HeatmapSettings &settings);
  bool request_define_functions_from_coverage();

  static CoverageWorkspaceController *find(BinaryNinja::BinaryView *view);
  static void register_controller(BinaryNinja::BinaryView *view,
                                  CoverageWorkspaceController *controller);
  static void unregister_controller(BinaryNinja::BinaryView *view);

private:
  using ViewKey = BNBinaryView *;
  struct ControllerState {
    std::atomic<bool> alive{true};
    CoverageWorkspaceController *controller = nullptr;
  };

  struct TraceRecord {
    uint64_t id = 0;
    std::string alias;
    coverage::CoverageTrace trace;
    core::CoverageIndex index;
    coverage::CoverageStats stats;
  };

  struct CompositionResult {
    core::CoverageIndex index;
  };

  BinaryViewRef view_;
  CoverageWorkspaceView *view_ui_ = nullptr;
  std::shared_ptr<ControllerState> state_;
  coverage::CoverageParserRegistry parser_registry_;
  core::CoverageMapper mapper_;
  std::unique_ptr<CoveragePainter> painter_;
  std::vector<TraceRecord> traces_;
  std::optional<core::CoverageIndex> active_index_;
  std::atomic<uint64_t> compose_generation_{0};
  std::atomic<uint64_t> filter_generation_{0};
  uint64_t next_trace_id_ = 1;
  std::string expression_;
  std::string block_filter_;
  HighlightMode highlight_mode_ = HighlightMode::Plain;
  HighlightGranularity highlight_granularity_ =
      HighlightGranularity::Instruction;
  HeatmapSettings heatmap_settings_{};
  BinaryNinja::Ref<BinaryNinja::Logger> logger_;

  static void
  dispatch_ui(const std::shared_ptr<ControllerState> &state,
              const std::function<void(CoverageWorkspaceController &)> &action);
  void add_trace_result(TraceRecord record);
  void update_trace_view();
  void update_blocks_view(const std::vector<core::CoveredBlock> &blocks);
  void compose_expression_async(
      uint64_t generation, coverage::ComposePlan plan,
      std::unordered_map<std::string, coverage::CoverageDataset> datasets);
  void filter_blocks_async(uint64_t generation, core::BlockFilter filter,
                           std::vector<BlockSummary> blocks);
  void apply_active_highlights();
  std::string next_alias() const;

  static std::unordered_map<ViewKey, CoverageWorkspaceController *> registry_;
};

} // namespace binja::covex::ui
