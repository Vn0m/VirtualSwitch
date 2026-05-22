#include "switch_controller.hpp"
#include <QSettings>
#include <QRandomGenerator>

SwitchController::SwitchController(QObject* parent)
    : QObject(parent), process_(new QProcess(this)), tap_ip_(initTapIp()) {
    process_->setProcessChannelMode(QProcess::MergedChannels);
    connect(process_, &QProcess::readyReadStandardOutput, this, &SwitchController::onReadyRead);
}

QString SwitchController::tapIp() const {
    return tap_ip_;
}

QString SwitchController::initTapIp() {
    QSettings settings("AllBlue", "AllBlue");
    if (settings.contains("tap_ip"))
        return settings.value("tap_ip").toString();
    int octet = QRandomGenerator::global()->bounded(2, 255);
    QString ip = QString("10.0.0.%1").arg(octet);
    settings.setValue("tap_ip", ip);
    return ip;
}

void SwitchController::start(const QStringList& peers) {
    stop();

    QStringList args = {
        "run", "--rm", "--name", "allblue-node",
        "--cap-add", "NET_ADMIN",
        "--device", "/dev/net/tun",
        "-e", "TAP_IP=" + tap_ip_,
        "-e", "PEERS=" + peers.join(","),
        "-p", "5000:5000/udp",
        "allblue-vswitch"
    };

    process_->start("docker", args);
}

void SwitchController::stop() {
    if (process_->state() != QProcess::NotRunning) {
        QProcess::startDetached("docker", {"stop", "allblue-node"});
        process_->terminate();
        process_->waitForFinished(3000);
    }
}

void SwitchController::onReadyRead() {
    while (process_->canReadLine()) {
        QString line = QString::fromUtf8(process_->readLine()).trimmed();
        if (line.isEmpty()) continue;

        if (line.startsWith("Your public address: ")) {
            QString addr = line.mid(21).split("  ").first();
            emit publicAddressDiscovered(addr);
        }

        emit outputReceived(line);
    }
}
