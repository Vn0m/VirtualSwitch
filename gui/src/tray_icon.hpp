#pragma once
#include <QSystemTrayIcon>

class TrayIcon : public QSystemTrayIcon {
    Q_OBJECT

public:
    explicit TrayIcon(QObject* parent = nullptr);
};
