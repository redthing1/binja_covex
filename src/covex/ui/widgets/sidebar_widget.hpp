#pragma once

#include <QToolButton>
#include <QWidget>
#include <memory>

#include "binaryninjaapi.h"
#include "sidebarwidget.h"
#include "viewframe.h"

namespace binja::covex::ui {

class CoverageWorkspaceController;

class CovexSidebarWidget final : public SidebarWidget {
public:
  CovexSidebarWidget(const QString &name, ViewFrame *frame, BinaryViewRef data);
  ~CovexSidebarWidget() override;

  QWidget *widget();
  bool request_load();
  void request_clear();
  void notifyThemeChanged() override;
  void notifyFontChanged() override;
  void notifyViewChanged(ViewFrame *frame) override;

private:
  void build_ui();
  void connect_signals();
  void apply_theme();

  BinaryViewRef m_data;
  ViewFrame *m_frame = nullptr;
  QToolButton *m_load_button = nullptr;
  QToolButton *m_clear_button = nullptr;
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
