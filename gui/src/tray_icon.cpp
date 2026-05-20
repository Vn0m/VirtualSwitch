#include "tray_icon.hpp"
#include <QMenu>

TrayIcon::TrayIcon(QObject* parent) : QSystemTrayIcon(parent) {
    auto* menu = new QMenu();
    menu->addAction("Show");
    menu->addAction("Quit");
    setContextMenu(menu);
}
