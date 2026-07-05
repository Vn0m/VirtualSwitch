#include "switch_controller.hpp"
#include <QSettings>
#include <QRandomGenerator>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QDir>

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
    if (settings.contains("tap_ip")) {
        return settings.value("tap_ip").toString();
    }
    int octet = QRandomGenerator::global()->bounded(2, 255);
    QString ip = QString("10.0.0.%1").arg(octet);
    settings.setValue("tap_ip", ip);
    return ip;
}

QString SwitchController::utunHelperPath() const {
    return QCoreApplication::applicationDirPath() + "/allblue-utun";
}

void SwitchController::start(const QStringList& peers) {
    stop();

    QStringList args = {
        "run", "--rm", "--name", "allblue-node",
        "--privileged",
        "--device", "/dev/net/tun",
        "-e", "TAP_IP=" + tap_ip_,
        "-e", "PEERS=" + peers.join(","),
        "-p", "5000:5000/udp",
        "-p", "127.0.0.1:1194:1194/udp",
        "allblue-vswitch"
    };

    process_->start("docker", args);
    launchTunnel();
}

void SwitchController::launchTunnel() {
    static const QRegularExpression ipv4(R"(^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}$)");
    if (!ipv4.match(tap_ip_).hasMatch()) {
        emit outputReceived("error: invalid virtual IP; tunnel not started");
        return;
    }

    QString helper = utunHelperPath();
    if (helper.contains('\'') || helper.contains('"') || helper.contains('\\')) {
        emit outputReceived("error: invalid helper path; tunnel not started");
        return;
    }

    QString log = QDir::homePath() + "/Library/Logs/allblue-utun.log";
    QString shell = "'" + helper + "' " + tap_ip_ + " >> '" + log + "' 2>&1 &";
    QString script = "do shell script \"" + shell + "\" with administrator privileges";
    QProcess::startDetached("osascript", {"-e", script});
}

void SwitchController::killTunnel() {
    QString script = "do shell script \"pkill -f allblue-utun\" with administrator privileges";
    QProcess::startDetached("osascript", {"-e", script});
}

void SwitchController::stop() {
    if (process_->state() != QProcess::NotRunning) {
        QProcess::startDetached("docker", {"stop", "allblue-node"});
        process_->terminate();
        process_->waitForFinished(3000);
        killTunnel();
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
