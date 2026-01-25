#include "covex/ui/controllers/workspace_controller.hpp"

#include <exception>
#include <thread>

#include "binaryninjaapi.h"
#include "covex/core/logging.hpp"

namespace binja::covex::ui {

std::unordered_map<CoverageWorkspaceController::ViewKey,
                   CoverageWorkspaceController *>
    CoverageWorkspaceController::registry_;

namespace {

BNBinaryView *view_key(BinaryNinja::BinaryView *view) {
  if (!view) {
    return nullptr;
  }
  return view->GetObject();
}

std::string make_alias(size_t index) {
  std::string alias;
  size_t n = index;
  while (true) {
    const size_t rem = n % 26;
    alias.insert(alias.begin(), static_cast<char>('A' + rem));
    if (n < 26) {
      break;
    }
    n = (n / 26) - 1;
  }
  return alias;
}

} // namespace

CoverageWorkspaceController::CoverageWorkspaceController(
    BinaryViewRef view, CoverageWorkspaceView &view_ui)
    : view_(view), view_ui_(&view_ui) {
  state_ = std::make_shared<ControllerState>();
  state_->controller = this;
  register_controller(view_.GetPtr(), this);
  parser_registry_.register_parser(std::make_unique<coverage::DrcovParser>());
  painter_ = std::make_unique<CoveragePainter>(view_);
  logger_ = log::logger(view_, log::kLogger);
}

CoverageWorkspaceController::~CoverageWorkspaceController() {
  if (state_) {
    state_->alive = false;
    state_->controller = nullptr;
  }
  unregister_controller(view_.GetPtr());
}

CoverageWorkspaceController *
CoverageWorkspaceController::find(BinaryNinja::BinaryView *view) {
  const auto key = view_key(view);
  if (!key) {
    return nullptr;
  }
  auto it = registry_.find(key);
  if (it == registry_.end()) {
    return nullptr;
  }
  return it->second;
}

void CoverageWorkspaceController::register_controller(
    BinaryNinja::BinaryView *view, CoverageWorkspaceController *controller) {
  const auto key = view_key(view);
  if (!key || !controller) {
    return;
  }
  registry_[key] = controller;
}

void CoverageWorkspaceController::unregister_controller(
    BinaryNinja::BinaryView *view) {
  const auto key = view_key(view);
  if (!key) {
    return;
  }
  registry_.erase(key);
}

void CoverageWorkspaceController::dispatch_ui(
    const std::shared_ptr<ControllerState> &state,
    const std::function<void(CoverageWorkspaceController &)> &action) {
  BinaryNinja::ExecuteOnMainThread([state, action]() {
    if (!state || !state->alive.load() || !state->controller) {
      return;
    }
    action(*state->controller);
  });
}

bool CoverageWorkspaceController::prompt_load() {
  if (!view_) {
    return false;
  }
  std::string path;
  const std::string filter = "Coverage Files (*.drcov *.cov);;All Files (*)";
  if (!BinaryNinja::GetOpenFileNameInput(path, "Open coverage file", filter)) {
    return false;
  }
  return load_trace_file(path);
}

bool CoverageWorkspaceController::load_trace_file(const std::string &path) {
  if (!view_) {
    return false;
  }

  BinaryNinja::Ref<BinaryNinja::BackgroundTask> task =
      new BinaryNinja::BackgroundTask("CovEx: Loading coverage...", false);
  auto view = view_;
  auto logger = logger_;
  auto *parser_registry = &parser_registry_;
  auto *mapper = &mapper_;
  auto state = state_;

  std::thread([state, task, view, logger, parser_registry, mapper, path]() {
    if (logger) {
      logger->LogInfoF("Loading coverage file: {}", path);
    }
    std::optional<coverage::CoverageTrace> parsed;
    try {
      task->SetProgressText("CovEx: Parsing coverage...");
      parsed = parser_registry->parse_first_match(path);
    } catch (const std::exception &err) {
      if (logger) {
        logger->LogErrorForExceptionF(err, "Failed to parse coverage file: {}",
                                      path);
      }
      task->Finish();
      return;
    }

    if (!parsed) {
      if (logger) {
        logger->LogWarnF("Unsupported coverage file: {}", path);
      }
      task->Finish();
      return;
    }

    auto trace = std::move(*parsed);
    const auto trace_spans = trace.spans.size();
    const auto trace_modules = trace.modules.size();
    const bool trace_hitcounts = trace.has_hitcounts;
    if (logger) {
      logger->LogInfoF("Parsed coverage: spans={} modules={} hitcounts={}",
                       trace_spans, trace_modules,
                       trace_hitcounts ? "yes" : "no");
    }

    task->SetProgressText("CovEx: Mapping coverage...");
    auto result = mapper->map_trace(trace, view);

    TraceRecord record;
    record.id = 0;
    record.alias.clear();
    record.trace = std::move(trace);
    record.dataset = std::move(result.dataset);
    record.blocks = std::move(result.blocks);
    record.invalid_addresses = std::move(result.invalid_addresses);
    record.diagnostics = std::move(result.diagnostics);
    record.stats = record.dataset.stats();

    if (logger) {
      const auto &diag = record.diagnostics;
      if (diag.matched_module_id) {
        const std::string reason =
            diag.match_reason.empty() ? "unknown" : diag.match_reason;
        if (diag.matched_module_base) {
          logger->LogInfoF("Module match ({}) id={} rebasing by slide={:+#x} "
                           "(view_base=0x{:x} module_base=0x{:x}) path={}",
                           reason, *diag.matched_module_id, diag.slide,
                           diag.view_image_base, *diag.matched_module_base,
                           diag.matched_module_path);
        } else {
          logger->LogInfoF("Module match ({}) id={} slide={:+#x} path={}",
                           reason, *diag.matched_module_id, diag.slide,
                           diag.matched_module_path);
        }
      } else if (trace_modules != 0) {
        logger->LogWarn(
            "No module matched BinaryView; coverage addresses left as-is");
      }
      logger->LogInfoF("Mapped spans: total={} mapped={} skipped={} invalid={}",
                       diag.spans_total, diag.spans_mapped, diag.spans_skipped,
                       record.invalid_addresses.size());
      logger->LogInfoF("Mapped addresses: unique={} total={}",
                       record.stats.unique_addresses, record.stats.total_hits);
    }

    task->Finish();

    dispatch_ui(state, [record = std::move(record)](
                           CoverageWorkspaceController &controller) mutable {
      controller.add_trace_result(std::move(record));
    });
  }).detach();

  return true;
}

void CoverageWorkspaceController::add_trace_result(TraceRecord record) {
  record.id = next_trace_id_++;
  record.alias = next_alias();
  traces_.push_back(std::move(record));
  update_trace_view();
  set_expression(expression_);
}

void CoverageWorkspaceController::update_trace_view() {
  if (!view_ui_) {
    return;
  }
  std::vector<TraceSummary> summaries;
  summaries.reserve(traces_.size());
  for (const auto &trace : traces_) {
    TraceSummary summary;
    summary.alias = trace.alias;
    summary.name = trace.trace.name;
    summary.spans = trace.trace.spans.size();
    summary.unique_addresses = trace.stats.unique_addresses;
    summary.total_hits = trace.stats.total_hits;
    summary.has_hitcounts = trace.trace.has_hitcounts;
    summaries.push_back(std::move(summary));
  }
  view_ui_->set_traces(summaries);
}

std::string CoverageWorkspaceController::next_alias() const {
  return make_alias(traces_.size());
}

void CoverageWorkspaceController::set_expression(
    const std::string &expression) {
  expression_ = expression;
  if (!view_ui_) {
    return;
  }

  const auto generation = compose_generation_.fetch_add(1) + 1;

  if (expression.empty()) {
    view_ui_->clear_expression_error();
    if (traces_.empty()) {
      active_dataset_.reset();
      active_blocks_.clear();
      if (painter_) {
        painter_->clear();
      }
      update_blocks_view(active_blocks_);
      return;
    }
    if (!traces_.empty()) {
      active_dataset_ = traces_.front().dataset;
      active_blocks_ = traces_.front().blocks;
      apply_active_highlights();
      update_blocks_view(active_blocks_);
    }
    return;
  }

  auto parsed = coverage::parse_expression(expression);
  if (std::holds_alternative<coverage::ComposeError>(parsed)) {
    const auto &err = std::get<coverage::ComposeError>(parsed);
    view_ui_->show_expression_error(err.message);
    if (logger_) {
      logger_->LogWarnF("Composition parse error: {}", err.message);
    }
    return;
  }

  view_ui_->clear_expression_error();
  auto plan = std::get<coverage::ComposePlan>(std::move(parsed));

  std::unordered_map<std::string, coverage::CoverageDataset> datasets;
  datasets.reserve(traces_.size());
  for (const auto &trace : traces_) {
    datasets.emplace(trace.alias, trace.dataset);
  }

  compose_expression_async(generation, std::move(plan), std::move(datasets));
}

void CoverageWorkspaceController::compose_expression_async(
    uint64_t generation, coverage::ComposePlan plan,
    std::unordered_map<std::string, coverage::CoverageDataset> datasets) {
  BinaryNinja::Ref<BinaryNinja::BackgroundTask> task =
      new BinaryNinja::BackgroundTask("CovEx: Composing coverage...", false);
  auto logger = logger_;
  auto view = view_;
  auto *mapper = &mapper_;
  auto state = state_;

  std::thread([state, generation, plan = std::move(plan),
               datasets = std::move(datasets), task, logger, view,
               mapper]() mutable {
    task->SetProgressText("CovEx: Evaluating composition...");
    auto composed = coverage::evaluate_expression(plan, datasets);
    if (std::holds_alternative<coverage::ComposeError>(composed)) {
      const auto &err = std::get<coverage::ComposeError>(composed);
      if (logger) {
        logger->LogWarnF("Composition error: {}", err.message);
      }
      std::string message = err.message;
      task->Finish();
      dispatch_ui(state, [generation,
                          message](CoverageWorkspaceController &controller) {
        if (generation != controller.compose_generation_.load()) {
          return;
        }
        if (controller.view_ui_) {
          controller.view_ui_->show_expression_error(message);
        }
      });
      return;
    }

    task->SetProgressText("CovEx: Mapping composition...");
    auto dataset = std::get<coverage::CoverageDataset>(std::move(composed));
    auto result = mapper->map_dataset(dataset, view);
    CompositionResult composed_result;
    composed_result.dataset = std::move(result.dataset);
    composed_result.blocks = std::move(result.blocks);

    task->Finish();

    dispatch_ui(
        state, [generation, composed_result = std::move(composed_result)](
                   CoverageWorkspaceController &controller) mutable {
          if (generation != controller.compose_generation_.load()) {
            return;
          }
          if (controller.view_ui_) {
            controller.view_ui_->clear_expression_error();
          }
          controller.active_dataset_ = std::move(composed_result.dataset);
          controller.active_blocks_ = std::move(composed_result.blocks);
          controller.apply_active_highlights();
          controller.update_blocks_view(controller.active_blocks_);
        });
  }).detach();
}

void CoverageWorkspaceController::set_block_filter(
    const std::string &filter_text) {
  block_filter_ = filter_text;
  const auto generation = filter_generation_.fetch_add(1) + 1;
  if (active_blocks_.empty()) {
    update_blocks_view(active_blocks_);
    return;
  }
  if (filter_text.empty()) {
    update_blocks_view(active_blocks_);
    return;
  }
  auto parsed = core::parse_block_filter(filter_text);
  if (std::holds_alternative<core::BlockFilterError>(parsed)) {
    const auto &err = std::get<core::BlockFilterError>(parsed);
    if (logger_) {
      logger_->LogWarnF("Block filter error: {}", err.message);
    }
    return;
  }

  auto filter = std::get<core::BlockFilter>(std::move(parsed));
  std::vector<BlockSummary> summaries;
  summaries.reserve(active_blocks_.size());
  for (const auto &block : active_blocks_) {
    BlockSummary summary;
    summary.address = block.start;
    summary.size = block.size;
    summary.hits = block.hits;
    summary.function = block.function;
    summaries.push_back(std::move(summary));
  }

  filter_blocks_async(generation, std::move(filter), std::move(summaries));
}

void CoverageWorkspaceController::set_highlight_mode(HighlightMode mode) {
  highlight_mode_ = mode;
  apply_active_highlights();
}

void CoverageWorkspaceController::set_granularity(
    HighlightGranularity granularity) {
  highlight_granularity_ = granularity;
  apply_active_highlights();
}

void CoverageWorkspaceController::set_heatmap_settings(
    const HeatmapSettings &settings) {
  heatmap_settings_ = settings;
  if (highlight_mode_ == HighlightMode::Heatmap) {
    apply_active_highlights();
  }
}

void CoverageWorkspaceController::filter_blocks_async(
    uint64_t generation, core::BlockFilter filter,
    std::vector<BlockSummary> blocks) {
  BinaryNinja::Ref<BinaryNinja::BackgroundTask> task =
      new BinaryNinja::BackgroundTask("CovEx: Filtering blocks...", false);
  auto state = state_;

  std::thread([state, generation, filter = std::move(filter),
               blocks = std::move(blocks), task]() mutable {
    std::vector<BlockSummary> filtered;
    filtered.reserve(blocks.size());
    for (const auto &block : blocks) {
      core::BlockFilterContext ctx;
      ctx.address = block.address;
      ctx.size = block.size;
      ctx.hits = block.hits;
      ctx.function = block.function;
      if (filter.matches(ctx)) {
        filtered.push_back(block);
      }
    }
    task->Finish();

    dispatch_ui(state, [generation, filtered = std::move(filtered)](
                           CoverageWorkspaceController &controller) mutable {
      if (generation != controller.filter_generation_.load()) {
        return;
      }
      if (!controller.view_ui_) {
        return;
      }
      controller.view_ui_->set_blocks(filtered);
    });
  }).detach();
}

void CoverageWorkspaceController::update_blocks_view(
    const std::vector<core::CoveredBlock> &blocks) {
  if (!view_ui_) {
    return;
  }
  std::vector<BlockSummary> summaries;
  summaries.reserve(blocks.size());
  for (const auto &block : blocks) {
    BlockSummary summary;
    summary.address = block.start;
    summary.size = block.size;
    summary.hits = block.hits;
    summary.function = block.function;
    summaries.push_back(std::move(summary));
  }
  view_ui_->set_blocks(summaries);
}

void CoverageWorkspaceController::apply_active_highlights() {
  if (!painter_ || !active_dataset_) {
    return;
  }
  if (highlight_mode_ == HighlightMode::Heatmap) {
    painter_->apply_heatmap(*active_dataset_, highlight_granularity_,
                            heatmap_settings_);
  } else {
    painter_->apply_plain(*active_dataset_, highlight_granularity_);
  }
}

void CoverageWorkspaceController::clear_highlights() {
  if (painter_) {
    painter_->clear();
  }
  if (logger_) {
    logger_->LogInfo("Cleared coverage highlights");
  }
}

} // namespace binja::covex::ui
