#include "covex/ui/widgets/sidebar_widget.hpp"

#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QVBoxLayout>

#include "covex/ui/theme/icon_loader.hpp"
#include "theme.h"
#include "uicontext.h"

namespace binja::covex::ui {

CovexSidebarWidget::CovexSidebarWidget(const QString &name, ViewFrame *frame,
                                       BinaryViewRef data)
    : SidebarWidget(name), m_data(data), m_frame(frame) {
  m_trace_model = new TraceTableModel(this);
  m_block_model = new BlockTableModel(this);
  m_controller = std::make_unique<CoverageWorkspaceController>(m_data, *this);
  build_ui();
  connect_signals();
  apply_theme();
}

CovexSidebarWidget::~CovexSidebarWidget() = default;

QWidget *CovexSidebarWidget::widget() { return this; }

bool CovexSidebarWidget::request_load() {
  if (!m_controller) {
    return false;
  }
  return m_controller->prompt_load();
}

void CovexSidebarWidget::request_clear() {
  if (!m_controller) {
    return;
  }
  m_controller->clear_highlights();
}

void CovexSidebarWidget::notifyThemeChanged() { apply_theme(); }

void CovexSidebarWidget::notifyFontChanged() { apply_theme(); }

void CovexSidebarWidget::notifyViewChanged(ViewFrame *frame) {
  m_frame = frame;
}

void CovexSidebarWidget::set_traces(const std::vector<TraceSummary> &traces) {
  if (!m_trace_model) {
    return;
  }
  std::vector<TraceRow> rows;
  rows.reserve(traces.size());
  for (const auto &trace : traces) {
    TraceRow row;
    row.alias = QString::fromStdString(trace.alias);
    row.name = QString::fromStdString(trace.name);
    row.spans = trace.spans;
    row.unique_addresses = trace.unique_addresses;
    row.total_hits = trace.total_hits;
    row.has_hitcounts = trace.has_hitcounts;
    rows.push_back(std::move(row));
  }
  m_trace_model->set_traces(std::move(rows));
}

void CovexSidebarWidget::set_blocks(const std::vector<BlockSummary> &blocks) {
  if (!m_block_model) {
    return;
  }
  std::vector<BlockRow> rows;
  rows.reserve(blocks.size());
  for (const auto &block : blocks) {
    BlockRow row;
    row.address = block.address;
    row.size = block.size;
    row.hits = block.hits;
    row.function = QString::fromStdString(block.function);
    rows.push_back(std::move(row));
  }
  m_block_model->set_blocks(std::move(rows));
}

void CovexSidebarWidget::show_expression_error(const std::string &message) {
  if (!m_expression_error) {
    return;
  }
  m_expression_error->setText(QString::fromStdString(message));
  m_expression_error->setVisible(true);
}

void CovexSidebarWidget::clear_expression_error() {
  if (!m_expression_error) {
    return;
  }
  m_expression_error->clear();
  m_expression_error->setVisible(false);
}

void CovexSidebarWidget::build_ui() {
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(8);

  auto *toolbar = new QWidget(this);
  auto *toolbar_layout = new QHBoxLayout(toolbar);
  toolbar_layout->setContentsMargins(0, 0, 0, 0);
  toolbar_layout->setSpacing(6);

  m_load_button = new QToolButton(this);
  m_load_button->setText("Load");
  m_load_button->setAutoRaise(true);
  m_load_button->setToolTip("Load coverage file");

  m_clear_button = new QToolButton(this);
  m_clear_button->setText("Clear");
  m_clear_button->setAutoRaise(true);
  m_clear_button->setToolTip("Clear coverage highlights");

  toolbar_layout->addWidget(m_load_button);
  toolbar_layout->addWidget(m_clear_button);
  toolbar_layout->addStretch();
  toolbar->setLayout(toolbar_layout);

  m_tabs = new QTabWidget(this);

  auto *traces_tab = new QWidget(this);
  auto *traces_layout = new QVBoxLayout(traces_tab);
  traces_layout->setContentsMargins(0, 0, 0, 0);
  traces_layout->setSpacing(6);

  auto *expression_row = new QWidget(traces_tab);
  auto *expression_layout = new QHBoxLayout(expression_row);
  expression_layout->setContentsMargins(0, 0, 0, 0);
  expression_layout->setSpacing(6);

  auto *expression_label = new QLabel("Expression", expression_row);
  m_expression_input = new QLineEdit(expression_row);

  expression_layout->addWidget(expression_label);
  expression_layout->addWidget(m_expression_input, 1);
  expression_row->setLayout(expression_layout);

  m_expression_error = new QLabel(traces_tab);
  m_expression_error->setVisible(false);
  m_expression_error->setWordWrap(true);

  m_traces_table = new QTableView(traces_tab);
  m_traces_table->setModel(m_trace_model);
  m_traces_table->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_traces_table->setSelectionMode(QAbstractItemView::SingleSelection);
  m_traces_table->horizontalHeader()->setStretchLastSection(true);
  m_traces_table->verticalHeader()->setVisible(false);

  traces_layout->addWidget(expression_row);
  traces_layout->addWidget(m_expression_error);
  traces_layout->addWidget(m_traces_table, 1);
  traces_tab->setLayout(traces_layout);

  auto *blocks_tab = new QWidget(this);
  auto *blocks_layout = new QVBoxLayout(blocks_tab);
  blocks_layout->setContentsMargins(0, 0, 0, 0);
  blocks_layout->setSpacing(6);

  auto *filter_row = new QWidget(blocks_tab);
  auto *filter_layout = new QHBoxLayout(filter_row);
  filter_layout->setContentsMargins(0, 0, 0, 0);
  filter_layout->setSpacing(6);

  auto *filter_label = new QLabel("Filter", filter_row);
  m_block_filter = new QLineEdit(filter_row);

  filter_layout->addWidget(filter_label);
  filter_layout->addWidget(m_block_filter, 1);
  filter_row->setLayout(filter_layout);

  m_blocks_table = new QTableView(blocks_tab);
  m_blocks_table->setModel(m_block_model);
  m_blocks_table->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_blocks_table->setSelectionMode(QAbstractItemView::SingleSelection);
  m_blocks_table->horizontalHeader()->setStretchLastSection(true);
  m_blocks_table->verticalHeader()->setVisible(false);

  blocks_layout->addWidget(filter_row);
  blocks_layout->addWidget(m_blocks_table, 1);
  blocks_tab->setLayout(blocks_layout);

  auto *overview_tab = new QWidget(this);
  auto *overview_layout = new QVBoxLayout(overview_tab);
  overview_layout->setContentsMargins(0, 0, 0, 0);
  overview_layout->setSpacing(8);

  auto *highlight_row = new QWidget(overview_tab);
  auto *highlight_layout = new QHBoxLayout(highlight_row);
  highlight_layout->setContentsMargins(0, 0, 0, 0);
  highlight_layout->setSpacing(6);

  auto *mode_label = new QLabel("Mode", highlight_row);
  m_mode_combo = new QComboBox(highlight_row);
  m_mode_combo->addItem("Plain");
  m_mode_combo->addItem("Heatmap");

  highlight_layout->addWidget(mode_label);
  highlight_layout->addWidget(m_mode_combo, 1);
  highlight_row->setLayout(highlight_layout);

  auto *granularity_row = new QWidget(overview_tab);
  auto *granularity_layout = new QHBoxLayout(granularity_row);
  granularity_layout->setContentsMargins(0, 0, 0, 0);
  granularity_layout->setSpacing(6);

  auto *granularity_label = new QLabel("Granularity", granularity_row);
  m_granularity_combo = new QComboBox(granularity_row);
  m_granularity_combo->addItem("Instruction");
  m_granularity_combo->addItem("Basic Block");

  granularity_layout->addWidget(granularity_label);
  granularity_layout->addWidget(m_granularity_combo, 1);
  granularity_row->setLayout(granularity_layout);

  auto *heatmap_row = new QWidget(overview_tab);
  auto *heatmap_layout = new QHBoxLayout(heatmap_row);
  heatmap_layout->setContentsMargins(0, 0, 0, 0);
  heatmap_layout->setSpacing(6);

  auto *percent_label = new QLabel("Heatmap Cap", heatmap_row);
  m_percentile_spin = new QSpinBox(heatmap_row);
  m_percentile_spin->setRange(50, 100);
  m_percentile_spin->setValue(95);
  m_percentile_spin->setSuffix("%");

  heatmap_layout->addWidget(percent_label);
  heatmap_layout->addWidget(m_percentile_spin, 1);
  heatmap_row->setLayout(heatmap_layout);

  auto *log_row = new QWidget(overview_tab);
  auto *log_layout = new QHBoxLayout(log_row);
  log_layout->setContentsMargins(0, 0, 0, 0);
  log_layout->setSpacing(6);

  auto *log_label = new QLabel("Log Scale", log_row);
  m_log_scale = new QCheckBox(log_row);
  m_log_scale->setChecked(true);

  log_layout->addWidget(log_label);
  log_layout->addWidget(m_log_scale, 1);
  log_row->setLayout(log_layout);

  auto *alpha_row = new QWidget(overview_tab);
  auto *alpha_layout = new QHBoxLayout(alpha_row);
  alpha_layout->setContentsMargins(0, 0, 0, 0);
  alpha_layout->setSpacing(6);

  auto *alpha_label = new QLabel("Highlight Alpha", alpha_row);
  m_alpha_spin = new QSpinBox(alpha_row);
  m_alpha_spin->setRange(16, 255);
  m_alpha_spin->setValue(255);

  alpha_layout->addWidget(alpha_label);
  alpha_layout->addWidget(m_alpha_spin, 1);
  alpha_row->setLayout(alpha_layout);

  overview_layout->addWidget(highlight_row);
  overview_layout->addWidget(granularity_row);
  overview_layout->addWidget(heatmap_row);
  overview_layout->addWidget(log_row);
  overview_layout->addWidget(alpha_row);
  overview_layout->addStretch();
  overview_tab->setLayout(overview_layout);

  m_tabs->addTab(traces_tab, "Traces");
  m_tabs->addTab(blocks_tab, "Blocks");
  m_tabs->addTab(overview_tab, "Overview");

  layout->addWidget(toolbar);
  layout->addWidget(m_tabs, 1);
  setLayout(layout);

  update_heatmap_controls(false);
}

void CovexSidebarWidget::connect_signals() {
  if (m_load_button) {
    QObject::connect(m_load_button, &QToolButton::clicked, this,
                     [this]() { request_load(); });
  }
  if (m_clear_button) {
    QObject::connect(m_clear_button, &QToolButton::clicked, this,
                     [this]() { request_clear(); });
  }

  m_expression_timer = new QTimer(this);
  m_expression_timer->setInterval(250);
  m_expression_timer->setSingleShot(true);
  QObject::connect(m_expression_timer, &QTimer::timeout, this,
                   [this]() { trigger_expression_update(); });
  if (m_expression_input) {
    QObject::connect(m_expression_input, &QLineEdit::textChanged, this,
                     [this]() {
                       if (m_expression_timer) {
                         m_expression_timer->start();
                       }
                     });
  }

  m_filter_timer = new QTimer(this);
  m_filter_timer->setInterval(200);
  m_filter_timer->setSingleShot(true);
  QObject::connect(m_filter_timer, &QTimer::timeout, this,
                   [this]() { trigger_filter_update(); });
  if (m_block_filter) {
    QObject::connect(m_block_filter, &QLineEdit::textChanged, this, [this]() {
      if (m_filter_timer) {
        m_filter_timer->start();
      }
    });
  }

  if (m_blocks_table) {
    QObject::connect(m_blocks_table, &QTableView::doubleClicked, this,
                     [this](const QModelIndex &index) {
                       if (!index.isValid() || !m_block_model) {
                         return;
                       }
                       const auto *row = m_block_model->block_at(index.row());
                       if (!row) {
                         return;
                       }
                       navigate_to_address(row->address);
                     });
  }

  if (m_mode_combo) {
    QObject::connect(
        m_mode_combo, &QComboBox::currentIndexChanged, this, [this](int index) {
          if (!m_controller) {
            return;
          }
          const auto mode =
              (index == 1) ? HighlightMode::Heatmap : HighlightMode::Plain;
          update_heatmap_controls(mode == HighlightMode::Heatmap);
          m_controller->set_highlight_mode(mode);
          if (mode == HighlightMode::Heatmap) {
            m_controller->set_heatmap_settings(current_heatmap_settings());
          }
        });
  }

  if (m_granularity_combo) {
    QObject::connect(m_granularity_combo, &QComboBox::currentIndexChanged, this,
                     [this](int index) {
                       if (!m_controller) {
                         return;
                       }
                       const auto granularity =
                           (index == 1) ? HighlightGranularity::BasicBlock
                                        : HighlightGranularity::Instruction;
                       m_controller->set_granularity(granularity);
                     });
  }

  auto heatmap_changed = [this]() {
    if (!m_controller) {
      return;
    }
    m_controller->set_heatmap_settings(current_heatmap_settings());
  };
  if (m_percentile_spin) {
    QObject::connect(m_percentile_spin,
                     QOverload<int>::of(&QSpinBox::valueChanged), this,
                     [heatmap_changed](int) { heatmap_changed(); });
  }
  if (m_log_scale) {
    QObject::connect(m_log_scale, &QCheckBox::toggled, this,
                     [heatmap_changed](bool) { heatmap_changed(); });
  }
  if (m_alpha_spin) {
    QObject::connect(m_alpha_spin, QOverload<int>::of(&QSpinBox::valueChanged),
                     this, [heatmap_changed](int) { heatmap_changed(); });
  }
}

void CovexSidebarWidget::apply_theme() {
  if (m_expression_error) {
    const QColor color = getThemeHighlightColor(RedHighlightColor);
    m_expression_error->setStyleSheet(
        QString("color: %1; font-size: 11px;").arg(color.name()));
  }
}

void CovexSidebarWidget::trigger_expression_update() {
  if (!m_controller || !m_expression_input) {
    return;
  }
  m_controller->set_expression(m_expression_input->text().toStdString());
}

void CovexSidebarWidget::trigger_filter_update() {
  if (!m_controller || !m_block_filter) {
    return;
  }
  m_controller->set_block_filter(m_block_filter->text().toStdString());
}

void CovexSidebarWidget::update_heatmap_controls(bool enabled) {
  if (m_percentile_spin) {
    m_percentile_spin->setEnabled(enabled);
  }
  if (m_log_scale) {
    m_log_scale->setEnabled(enabled);
  }
  if (m_alpha_spin) {
    m_alpha_spin->setEnabled(enabled);
  }
}

HeatmapSettings CovexSidebarWidget::current_heatmap_settings() const {
  HeatmapSettings settings;
  if (m_percentile_spin) {
    settings.percentile_cap = static_cast<uint32_t>(m_percentile_spin->value());
  }
  if (m_log_scale) {
    settings.log_scale = m_log_scale->isChecked();
  }
  if (m_alpha_spin) {
    settings.alpha = static_cast<uint8_t>(m_alpha_spin->value());
  }
  return settings;
}

void CovexSidebarWidget::navigate_to_address(uint64_t address) {
  UIContext *context = UIContext::contextForWidget(this);
  ViewFrame *frame = m_frame;
  if (!frame && context) {
    frame = context->getCurrentViewFrame();
  }
  if (!frame) {
    return;
  }
  frame->navigate(m_data, address, true, true);
  if (context) {
    context->refreshCurrentViewContents();
  }
}

CovexSidebarWidgetType::CovexSidebarWidgetType()
    : SidebarWidgetType(make_icon_image(":/covex/icons/blend.svg",
                                        getThemeColor(SidebarActiveIconColor)),
                        "CovEx") {}

SidebarWidget *CovexSidebarWidgetType::createWidget(ViewFrame *frame,
                                                    BinaryViewRef data) {
  return new CovexSidebarWidget("CovEx", frame, data);
}

SidebarWidgetLocation CovexSidebarWidgetType::defaultLocation() const {
  return SidebarWidgetLocation::RightContent;
}

SidebarContextSensitivity CovexSidebarWidgetType::contextSensitivity() const {
  return PerViewTypeSidebarContext;
}

SidebarIconVisibility CovexSidebarWidgetType::defaultIconVisibility() const {
  return AlwaysShowSidebarIcon;
}

} // namespace binja::covex::ui
