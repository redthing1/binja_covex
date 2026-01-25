#include "covex/ui/widgets/sidebar_widget.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>

#include "covex/ui/controllers/workspace_controller.hpp"
#include "covex/ui/theme/icon_loader.hpp"

namespace binja::covex::ui {

CovexSidebarWidget::CovexSidebarWidget(const QString &name, ViewFrame *frame,
                                       BinaryViewRef data)
    : SidebarWidget(name), m_data(data), m_frame(frame) {
  m_controller = std::make_unique<CoverageWorkspaceController>(m_data);
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

void CovexSidebarWidget::build_ui() {
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(6);

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

  layout->addWidget(toolbar);
  layout->addStretch();
  setLayout(layout);
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
  (void)m_controller;
}

void CovexSidebarWidget::apply_theme() { (void)this; }

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
