#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "binaryninjaapi.h"
#include "covex/core/coverage_mapper.hpp"
#include "covex/coverage/coverage_parser.hpp"
#include "covex/coverage/coverage_store.hpp"
#include "covex/coverage/drcov_reader.hpp"
#include "covex/ui/painting/coverage_painter.hpp"
#include "uitypes.h"

namespace binja::covex::ui {

class CoverageWorkspaceController final {
public:
  explicit CoverageWorkspaceController(BinaryViewRef view);
  ~CoverageWorkspaceController();

  BinaryViewRef view() const { return view_; }

  bool prompt_load();
  bool load_trace_file(const std::string &path);
  void clear_highlights();
  void rebuild_active_dataset();

  const std::optional<coverage::CoverageDataset> &active_dataset() const {
    return active_dataset_;
  }
  const std::vector<uint64_t> &invalid_addresses() const {
    return invalid_addresses_;
  }

  static CoverageWorkspaceController *find(BinaryNinja::BinaryView *view);
  static void register_controller(BinaryNinja::BinaryView *view,
                                  CoverageWorkspaceController *controller);
  static void unregister_controller(BinaryNinja::BinaryView *view);

private:
  using ViewKey = BNBinaryView *;

  BinaryViewRef view_;
  coverage::CoverageStore store_;
  coverage::CoverageParserRegistry parser_registry_;
  core::CoverageMapper mapper_;
  std::unique_ptr<CoveragePainter> painter_;
  std::optional<coverage::CoverageDataset> active_dataset_;
  std::vector<uint64_t> invalid_addresses_;
  BinaryNinja::Ref<BinaryNinja::Logger> logger_;

  static std::unordered_map<ViewKey, CoverageWorkspaceController *> registry_;
};

} // namespace binja::covex::ui
