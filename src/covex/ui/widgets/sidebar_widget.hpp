#pragma once

#include <QLabel>
#include <QWidget>

#include "binaryninjaapi.h"
#include "sidebarwidget.h"
#include "viewframe.h"

namespace binja::covex::ui {

class CovexSidebarWidget final : public SidebarWidget {
public:
  CovexSidebarWidget(const QString &name, ViewFrame *frame, BinaryViewRef data);
  ~CovexSidebarWidget() override;

  QWidget *widget();
  void notifyThemeChanged() override;
  void notifyFontChanged() override;
  void notifyViewChanged(ViewFrame *frame) override;

private:
  void build_ui();
  void apply_theme();

  BinaryViewRef m_data;
  ViewFrame *m_frame = nullptr;
  QLabel *m_title = nullptr;
  QLabel *m_status = nullptr;
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
