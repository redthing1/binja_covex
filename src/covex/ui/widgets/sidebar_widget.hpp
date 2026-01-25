#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableView>
#include <QTimer>
#include <QToolButton>
#include <QWidget>
#include <memory>

#include "binaryninjaapi.h"
#include "covex/ui/controllers/workspace_controller.hpp"
#include "covex/ui/models/block_table_model.hpp"
#include "covex/ui/models/trace_table_model.hpp"
#include "sidebarwidget.h"
#include "viewframe.h"

namespace binja::covex::ui {

class CovexSidebarWidget final : public SidebarWidget,
                                 public CoverageWorkspaceView {
public:
  CovexSidebarWidget(const QString &name, ViewFrame *frame, BinaryViewRef data);
  ~CovexSidebarWidget() override;

  QWidget *widget();
  bool request_load();
  void request_clear();
  void notifyThemeChanged() override;
  void notifyFontChanged() override;
  void notifyViewChanged(ViewFrame *frame) override;

  void set_traces(const std::vector<TraceSummary> &traces) override;
  void set_blocks(const std::vector<BlockSummary> &blocks) override;
  void show_expression_error(const std::string &message) override;
  void clear_expression_error() override;

private:
  void build_ui();
  void connect_signals();
  void apply_theme();
  void trigger_expression_update();
  void trigger_filter_update();
  void update_heatmap_controls(bool enabled);
  HeatmapSettings current_heatmap_settings() const;
  void navigate_to_address(uint64_t address);

  BinaryViewRef m_data;
  ViewFrame *m_frame = nullptr;
  QToolButton *m_load_button = nullptr;
  QToolButton *m_clear_button = nullptr;
  QTabWidget *m_tabs = nullptr;
  QTableView *m_traces_table = nullptr;
  QTableView *m_blocks_table = nullptr;
  QLineEdit *m_expression_input = nullptr;
  QLabel *m_expression_error = nullptr;
  QComboBox *m_mode_combo = nullptr;
  QComboBox *m_granularity_combo = nullptr;
  QSpinBox *m_percentile_spin = nullptr;
  QCheckBox *m_log_scale = nullptr;
  QSpinBox *m_alpha_spin = nullptr;
  QLineEdit *m_block_filter = nullptr;
  QTimer *m_expression_timer = nullptr;
  QTimer *m_filter_timer = nullptr;
  TraceTableModel *m_trace_model = nullptr;
  BlockTableModel *m_block_model = nullptr;
  std::unique_ptr<CoverageWorkspaceController> m_controller;
};

class CovexSidebarWidgetType final : public SidebarWidgetType {
public:
  CovexSidebarWidgetType();
  SidebarWidget *createWidget(ViewFrame *frame, BinaryViewRef data) override;
  SidebarWidgetLocation defaultLocation() const override;
  SidebarContextSensitivity contextSensitivity() const override;
  SidebarIconVisibility defaultIconVisibility() const override;
};

} // namespace binja::covex::ui
