#include "main_window.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("AllBlue");
    resize(400, 300);

    auto* central = new QWidget(this);
    setCentralWidget(central);

    auto* main_layout = new QVBoxLayout(central);
    address_label_ = new QLabel("Peer Adress:", this);
    peer_input_ = new QLineEdit(this);
    connect_btn_ = new QPushButton("Connect", this);
    log_area_ = new QTextEdit(this);

    peer_input_->setPlaceholderText("1.2.3.4:5000");
    log_area_->setReadOnly(true);

    auto* row = new QHBoxLayout();
    row->addWidget(peer_input_);
    row->addWidget(connect_btn_);

    main_layout->addWidget(address_label_);
    main_layout->addLayout(row);
    main_layout->addWidget(log_area_);
    
}
