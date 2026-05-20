#pragma once
#include <QObject>
#include <QProcess>
#include <QString>

class SwitchController : public QObject {
    Q_OBJECT

public:
    explicit SwitchController(QObject* parent = nullptr);

public slots:
    void start(const QString& peerAddress);
    void stop();

signals:
    void statusChanged(const QString& status);
    void outputReceived(const QString& line);

private:
    QProcess* process_;
};
