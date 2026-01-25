#include "binaryninjaapi.h"
#include "covex/core/logging.hpp"
#include "covex/ui/settings/covex_settings.hpp"
#include "covex/ui/widgets/sidebar_widget.hpp"
#include "sidebar.h"

using namespace BinaryNinja;

extern "C" {

BN_DECLARE_CORE_ABI_VERSION
BN_DECLARE_UI_ABI_VERSION

BINARYNINJAPLUGIN bool UIPluginInit() {
  binja::covex::ui::register_settings();
  PluginCommand::Register(
      "CovEx\\Load Coverage", "Load coverage file into CovEx",
      [](BinaryView *view) {
        Sidebar *sidebar = Sidebar::current();
        if (!sidebar) {
          return;
        }
        sidebar->activate("CovEx");
        QWidget *raw_widget = sidebar->widget("CovEx");
        auto *widget =
            dynamic_cast<binja::covex::ui::CovexSidebarWidget *>(raw_widget);
        if (widget) {
          widget->request_load();
          return;
        }
        if (auto logger =
                binja::covex::log::logger(view, binja::covex::log::kLogger)) {
          logger->LogWarn("CovEx sidebar unavailable for load");
        }
      },
      [](BinaryView *view) { return view != nullptr; });
  PluginCommand::Register(
      "CovEx\\Clear Highlights", "Clear CovEx coverage highlights",
      [](BinaryView *view) {
        Sidebar *sidebar = Sidebar::current();
        if (!sidebar) {
          return;
        }
        sidebar->activate("CovEx");
        QWidget *raw_widget = sidebar->widget("CovEx");
        auto *widget =
            dynamic_cast<binja::covex::ui::CovexSidebarWidget *>(raw_widget);
        if (widget) {
          widget->request_clear();
          return;
        }
        if (auto logger =
                binja::covex::log::logger(view, binja::covex::log::kLogger)) {
          logger->LogWarn("CovEx sidebar unavailable for clear");
        }
      },
      [](BinaryView *view) { return view != nullptr; });
  Sidebar::addSidebarWidgetType(new binja::covex::ui::CovexSidebarWidgetType());
  return true;
}
}
