#include "binaryninjaapi.h"
#include "covex/ui/settings/covex_settings.hpp"
#include "covex/ui/widgets/sidebar_widget.hpp"
#include "sidebar.h"

using namespace BinaryNinja;

extern "C" {

BN_DECLARE_CORE_ABI_VERSION
BN_DECLARE_UI_ABI_VERSION

BINARYNINJAPLUGIN bool UIPluginInit() {
  binja::covex::ui::register_settings();
  Sidebar::addSidebarWidgetType(new binja::covex::ui::CovexSidebarWidgetType());
  return true;
}
}
