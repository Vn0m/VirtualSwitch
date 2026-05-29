#include "switch_controller.hpp"
#include <QSettings>
#include <QRandomGenerator>
#include <QFile>
#include <QDir>
#include <QTextStream>

SwitchController::SwitchController(QObject* parent)
    : QObject(parent), process_(new QProcess(this)), tap_ip_(initTapIp()),
      ovpn_key_b64_(loadOrClearOvpnKey()) {
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

QString SwitchController::loadOrClearOvpnKey() {
    QSettings settings("AllBlue", "AllBlue");
    return settings.value("openvpn_key").toString();
}

void SwitchController::start(const QStringList& peers) {
    stop();
    public_ip_.clear();

    QStringList args = {
        "run", "--rm", "--name", "allblue-node",
        "--cap-add", "NET_ADMIN",
        "--cap-add", "SYS_ADMIN",
        "--device", "/dev/net/tun",
        "--sysctl", "net.ipv4.ip_forward=1",
        "-e", "TAP_IP=" + tap_ip_,
        "-e", "PEERS=" + peers.join(","),
        "-p", "5000:5000/udp",
        "-p", "1194:1194/udp",
    };

    if (!ovpn_key_b64_.isEmpty()) {
        args << "-e" << "OPENVPN_KEY=" + ovpn_key_b64_;
    }

    args << "allblue-vswitch";

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

        if (line.startsWith("OPENVPN_KEY_GENERATED:")) {
            onKeyGenerated(line.mid(22).trimmed());
            continue;
        }

        if (line.startsWith("Your public address: ")) {
            QString addr = line.mid(21).split("  ").first();
            public_ip_ = addr.split(":").first();
            emit publicAddressDiscovered(addr);
            buildOvpnConfig();
        }

        emit outputReceived(line);
    }
}

void SwitchController::onKeyGenerated(const QString& keyB64) {
    ovpn_key_b64_ = keyB64;
    QSettings settings("AllBlue", "AllBlue");
    settings.setValue("openvpn_key", keyB64);
    buildOvpnConfig();
}

void SwitchController::buildOvpnConfig() {
    if (public_ip_.isEmpty() || ovpn_key_b64_.isEmpty()) return;

    QByteArray keyBytes = QByteArray::fromBase64(ovpn_key_b64_.toUtf8());
    QString key = QString::fromUtf8(keyBytes);

    QString config =
        "dev tun\n"
        "proto udp\n"
        "remote 127.0.0.1 1194\n"
        "ifconfig " + tap_ip_ + " 10.255.0.1\n"
        "route 10.0.0.0 255.255.255.0\n"
        "<secret>\n" +
        key +
        "</secret>\n";

    QString path = QDir::tempPath() + "/allblue.ovpn";
    QFile f(path);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(config.toUtf8());
        emit ovpnConfigReady(path);
    }
}
