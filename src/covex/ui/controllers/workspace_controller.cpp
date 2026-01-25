#include "covex/ui/controllers/workspace_controller.hpp"

#include <exception>
#include <string>

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

} // namespace

CoverageWorkspaceController::CoverageWorkspaceController(BinaryViewRef view)
    : view_(view) {
  register_controller(view_.GetPtr(), this);
  parser_registry_.register_parser(std::make_unique<coverage::DrcovParser>());
  painter_ = std::make_unique<CoveragePainter>(view_);
  logger_ = log::logger(view_, log::kLogger);
}

CoverageWorkspaceController::~CoverageWorkspaceController() {
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

  struct TaskGuard {
    BinaryNinja::Ref<BinaryNinja::BackgroundTask> task;
    explicit TaskGuard(const std::string &text)
        : task(new BinaryNinja::BackgroundTask(text, false)) {}
    ~TaskGuard() {
      if (task) {
        task->Finish();
      }
    }
    void update(const std::string &text) {
      if (task) {
        task->SetProgressText(text);
      }
    }
  };

  TaskGuard task("CovEx: Loading coverage...");

  if (logger_) {
    logger_->LogInfoF("Loading coverage file: {}", path);
  }

  std::optional<coverage::CoverageTrace> parsed;
  try {
    task.update("CovEx: Parsing coverage...");
    parsed = parser_registry_.parse_first_match(path);
  } catch (const std::exception &err) {
    if (logger_) {
      logger_->LogErrorForExceptionF(err, "Failed to parse coverage file: {}",
                                     path);
    }
    return false;
  }

  if (!parsed) {
    if (logger_) {
      logger_->LogWarnF("Unsupported coverage file: {}", path);
    }
    return false;
  }

  auto trace = std::move(*parsed);
  const auto trace_spans = trace.spans.size();
  const auto trace_modules = trace.modules.size();
  const bool trace_hitcounts = trace.has_hitcounts;
  if (logger_) {
    logger_->LogInfoF("Parsed coverage: spans={} modules={} hitcounts={}",
                      trace_spans, trace_modules,
                      trace_hitcounts ? "yes" : "no");
  }

  task.update("CovEx: Mapping coverage...");
  auto result = mapper_.map_trace(trace, view_);
  store_.add_trace(std::move(trace));
  active_dataset_ = std::move(result.dataset);
  invalid_addresses_ = std::move(result.invalid_addresses);

  if (painter_) {
    painter_->apply_plain(*active_dataset_, HighlightGranularity::Instruction);
  }

  const auto stats = active_dataset_->stats();
  if (logger_) {
    const auto &diag = result.diagnostics;
    if (diag.matched_module_id) {
      const std::string reason =
          diag.match_reason.empty() ? "unknown" : diag.match_reason;
      if (diag.matched_module_base) {
        logger_->LogInfoF("Module match ({}) id={} rebasing by slide={:+#x} "
                          "(view_base=0x{:x} module_base=0x{:x}) path={}",
                          reason, *diag.matched_module_id, diag.slide,
                          diag.view_image_base, *diag.matched_module_base,
                          diag.matched_module_path);
      } else {
        logger_->LogInfoF("Module match ({}) id={} slide={:+#x} path={}",
                          reason, *diag.matched_module_id, diag.slide,
                          diag.matched_module_path);
      }
    } else if (trace_modules != 0) {
      logger_->LogWarn(
          "No module matched BinaryView; coverage addresses left as-is");
    }
    logger_->LogInfoF("Mapped spans: total={} mapped={} skipped={} invalid={}",
                      diag.spans_total, diag.spans_mapped, diag.spans_skipped,
                      invalid_addresses_.size());
    logger_->LogInfoF("Mapped addresses: unique={} total={}",
                      stats.unique_addresses, stats.total_hits);
  }

  return true;
}

void CoverageWorkspaceController::clear_highlights() {
  if (painter_) {
    painter_->clear();
  }
  if (logger_) {
    logger_->LogInfo("Cleared coverage highlights");
  }
}

void CoverageWorkspaceController::rebuild_active_dataset() {
  if (!view_ || !active_dataset_) {
    return;
  }
  auto result = mapper_.map_dataset(*active_dataset_, view_);
  active_dataset_ = std::move(result.dataset);
  invalid_addresses_ = std::move(result.invalid_addresses);
  if (painter_) {
    painter_->apply_plain(*active_dataset_, HighlightGranularity::Instruction);
  }
}

} // namespace binja::covex::ui
