#include "switch_controller.hpp"

SwitchController::SwitchController(QObject* parent)
    : QObject(parent), process_(new QProcess(this)) {
}

void SwitchController::start(const QString& peerAddress) {

}

void SwitchController::stop() {
    process_->terminate();
}
