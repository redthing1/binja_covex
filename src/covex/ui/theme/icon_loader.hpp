#pragma once

#include <QIcon>
#include <QImage>
#include <QPixmap>

#include "theme.h"

namespace binja::covex::ui {

QPixmap make_pixmap(const QString &resource_path, BNThemeColor color);
QPixmap make_pixmap(const QString &resource_path, const QColor &color);
QIcon make_icon(const QString &resource_path, BNThemeColor color);
QIcon make_icon(const QString &resource_path, const QColor &color);
QImage make_icon_image(const QString &resource_path, const QColor &color);

} // namespace binja::covex::ui
