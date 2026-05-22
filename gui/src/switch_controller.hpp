#pragma once
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

class SwitchController : public QObject {
    Q_OBJECT

public:
    explicit SwitchController(QObject* parent = nullptr);

    QString tapIp() const;

public slots:
    void start(const QStringList& peers);
    void stop();

signals:
    void publicAddressDiscovered(const QString& addr);
    void outputReceived(const QString& line);

private:
    QProcess* process_;
    QString tap_ip_;

    void onReadyRead();
    QString initTapIp();
};
