#include "covex/ui/widgets/sidebar_widget.hpp"

#include <QVBoxLayout>

#include "covex/ui/theme/icon_loader.hpp"
#include "theme.h"

namespace binja::covex::ui {

CovexSidebarWidget::CovexSidebarWidget(const QString &name, ViewFrame *frame,
                                       BinaryViewRef data)
    : SidebarWidget(name), m_data(data), m_frame(frame) {
  build_ui();
  apply_theme();
}

CovexSidebarWidget::~CovexSidebarWidget() = default;

QWidget *CovexSidebarWidget::widget() { return this; }

void CovexSidebarWidget::notifyThemeChanged() { apply_theme(); }

void CovexSidebarWidget::notifyFontChanged() { apply_theme(); }

void CovexSidebarWidget::notifyViewChanged(ViewFrame *frame) {
  m_frame = frame;
}

void CovexSidebarWidget::build_ui() {
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(6);

  m_title = new QLabel("Covex Coverage", this);
  m_title->setWordWrap(false);

  m_status = new QLabel("No coverage loaded", this);
  m_status->setWordWrap(true);

  layout->addWidget(m_title);
  layout->addWidget(m_status);
  layout->addStretch();
  setLayout(layout);
}

void CovexSidebarWidget::apply_theme() {
  if (m_status) {
    const QColor status_color = getThemeColor(CommentColor);
    m_status->setStyleSheet(QString("color: %1;").arg(status_color.name()));
  }
}

CovexSidebarWidgetType::CovexSidebarWidgetType()
    : SidebarWidgetType(make_icon_image(":/covex/icons/blend.svg",
                                        getThemeColor(SidebarActiveIconColor)),
                        "Covex") {}

SidebarWidget *CovexSidebarWidgetType::createWidget(ViewFrame *frame,
                                                    BinaryViewRef data) {
  return new CovexSidebarWidget("Covex", frame, data);
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
