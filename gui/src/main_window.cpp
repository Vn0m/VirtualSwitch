#include "main_window.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QFrame>
#include <QScrollArea>
#include <QApplication>
#include <QClipboard>
#include <QRegularExpression>
#include <QPixmap>

static QFrame* make_separator(QWidget* parent) {
    auto* sep = new QFrame(parent);
    sep->setFrameShape(QFrame::HLine);
    return sep;
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("AllBlue");
    resize(470, 510);

    controller_ = new SwitchController(this);

    connect(controller_, &SwitchController::publicAddressDiscovered, this, [this](const QString& addr) {
        own_address_->setText(addr);
        spinner_->setVisible(false);
        status_dot_->setStyleSheet("color: #4caf50; font-size: 9px;");
    });

    connect(controller_, &SwitchController::outputReceived, this, [this](const QString& line) {
        for (auto* log : peer_logs_)
            log->append(line);
    });

    auto* central = new QWidget(this);
    setCentralWidget(central);

    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(15, 18, 15, 18);
    root->setSpacing(0);

    status_dot_ = new QLabel("●", this);
    status_dot_->setStyleSheet("color: #b0b8c8; font-size: 9px;");
    status_label_ = new QLabel("No peers", this);
    status_label_->setStyleSheet("color: #909aaa; font-size: 14px;");
    spinner_ = new oclero::qlementine::LoadingSpinner(this);
    spinner_->setVisible(false);

    auto* logo = new QLabel(this);
    logo->setPixmap(QPixmap(":/resources/allbluelogo.png").scaled(50, 50, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    auto* status_row = new QHBoxLayout();
    status_row->setSpacing(6);
    status_row->addWidget(status_dot_);
    status_row->addWidget(status_label_);
    status_row->addStretch();
    status_row->addWidget(spinner_);
    status_row->addWidget(logo);
    root->addLayout(status_row);
    root->addSpacing(10);

    own_address_ = new QLabel("—", this);
    own_address_->setStyleSheet("font-family: monospace; font-size: 17px; font-weight: 500;");
    copy_btn_ = new QPushButton("Copy", this);
    copy_btn_->setStyleSheet("font-size: 12px;");

    auto* addr_row = new QHBoxLayout();
    addr_row->setSpacing(10);
    addr_row->addWidget(own_address_);
    addr_row->addStretch();
    addr_row->addWidget(copy_btn_);
    root->addLayout(addr_row);
    root->addSpacing(4);

    auto* addr_hint = new oclero::qlementine::Label("public address · share with peers", this);
    addr_hint->setRole(oclero::qlementine::TextRole::Caption);
    addr_hint->setStyleSheet("font-size: 13px;");
    root->addWidget(addr_hint);
    root->addSpacing(10);

    tap_address_ = new QLabel(controller_->tapIp(), this);
    tap_address_->setStyleSheet("font-family: monospace; font-size: 17px; font-weight: 500;");

    auto* tap_row = new QHBoxLayout();
    tap_row->setSpacing(10);
    tap_row->addWidget(tap_address_);
    tap_row->addStretch();
    root->addLayout(tap_row);
    root->addSpacing(4);

    auto* tap_hint = new oclero::qlementine::Label("virtual address · your IP on the network", this);
    tap_hint->setRole(oclero::qlementine::TextRole::Caption);
    tap_hint->setStyleSheet("font-size: 13px;");
    root->addWidget(tap_hint);
    root->addSpacing(16);
    root->addWidget(make_separator(central));
    root->addSpacing(14);

    tabs_ = new oclero::qlementine::SegmentedControl(this);
    tabs_->setItemsShouldExpand(true);
    tabs_->setFixedHeight(30);
    tabs_->addItem("Peers");
    tabs_->addItem("Activity");
    tabs_->setCurrentIndex(0);
    root->addWidget(tabs_);
    root->addSpacing(14);

    pages_ = new QStackedWidget(this);
    root->addWidget(pages_);

    auto* peers_page = new QWidget(pages_);
    auto* peers_layout = new QVBoxLayout(peers_page);
    peers_layout->setContentsMargins(0, 0, 0, 0);
    peers_layout->setSpacing(0);

    auto* add_btn = new QPushButton("+ Add peer", peers_page);
    add_btn->setStyleSheet("font-size: 12px;");
    auto* peers_header = new QHBoxLayout();
    peers_header->addWidget(new oclero::qlementine::Label("Peers", peers_page));
    peers_header->addStretch();
    peers_header->addWidget(add_btn);
    peers_layout->addLayout(peers_header);
    peers_layout->addSpacing(10);

    add_row_ = new QWidget(peers_page);
    peer_input_ = new oclero::qlementine::LineEdit(add_row_);
    peer_input_->setPlaceholderText("1.2.3.4:5000");
    peer_input_->setUseMonoSpaceFont(true);

    auto* add_layout = new QHBoxLayout(add_row_);
    add_layout->setContentsMargins(0, 0, 0, 10);
    add_layout->setSpacing(8);
    add_layout->addWidget(peer_input_);
    add_row_->setVisible(false);
    peers_layout->addWidget(add_row_);

    peer_stack_ = new QStackedWidget(peers_page);

    auto* empty_label = new QLabel("No peers yet\nClick + Add peer to connect", peer_stack_);
    empty_label->setAlignment(Qt::AlignCenter);
    empty_label->setStyleSheet("color: #b0b8c8;");
    peer_stack_->addWidget(empty_label);

    auto* scroll = new QScrollArea(peer_stack_);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    peer_rows_ = new QWidget(scroll);
    peer_rows_layout_ = new QVBoxLayout(peer_rows_);
    peer_rows_layout_->setContentsMargins(0, 0, 0, 0);
    peer_rows_layout_->setSpacing(2);
    peer_rows_layout_->addStretch();
    scroll->setWidget(peer_rows_);
    peer_stack_->addWidget(scroll);

    peer_stack_->setCurrentIndex(0);
    peers_layout->addWidget(peer_stack_);

    pages_->addWidget(peers_page);

    auto* log_page = new QWidget(pages_);
    auto* log_layout = new QVBoxLayout(log_page);
    log_layout->setContentsMargins(0, 0, 0, 0);

    log_stack_ = new QStackedWidget(log_page);
    auto* log_hint = new QLabel("Click a peer to view its activity", log_stack_);
    log_hint->setAlignment(Qt::AlignCenter);
    log_hint->setStyleSheet("color: #b0b8c8;");
    log_stack_->addWidget(log_hint);

    log_layout->addWidget(log_stack_);
    pages_->addWidget(log_page);

    connect(tabs_, &oclero::qlementine::SegmentedControl::currentIndexChanged,
            this, [this]() {
                pages_->setCurrentIndex(tabs_->currentIndex());
            });

    connect(add_btn, &QPushButton::clicked, this, [this]() {
        add_row_->setVisible(!add_row_->isVisible());
        if (add_row_->isVisible()) {
            peer_input_->setFocus();
        }
    });

    connect(peer_input_, &QLineEdit::returnPressed, this, [this]() {
        const auto addr = peer_input_->text().trimmed();

        static const QRegularExpression valid_format(
            R"(^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}:\d{1,5}$)");

        if (!valid_format.match(addr).hasMatch()) {
            peer_input_->setStatus(oclero::qlementine::Status::Error);
            return;
        }
        if (peer_addresses_.contains(addr)) {
            peer_input_->setStatus(oclero::qlementine::Status::Warning);
            return;
        }

        peer_input_->setStatus(oclero::qlementine::Status::Default);
        peer_addresses_.insert(addr);

        auto* log = new QTextEdit(log_stack_);
        log->setReadOnly(true);
        log->setPlaceholderText(addr + " · activity will appear here...");
        log_stack_->addWidget(log);
        peer_logs_[addr] = log;

        auto* row = new QPushButton(peer_rows_);
        row->setFlat(true);
        row->setMinimumHeight(36);
        auto* row_layout = new QHBoxLayout(row);
        row_layout->setContentsMargins(4, 8, 4, 8);
        row_layout->setSpacing(10);
        auto* dot = new QLabel("●", row);
        dot->setStyleSheet("color: #909090; font-size: 9px;");
        dot->setAttribute(Qt::WA_TransparentForMouseEvents);
        auto* addr_label = new QLabel(addr, row);
        addr_label->setStyleSheet("font-family: monospace; color: #333333;");
        addr_label->setAttribute(Qt::WA_TransparentForMouseEvents);
        row_layout->addWidget(dot);
        row_layout->addWidget(addr_label);
        row_layout->addStretch();
        connect(row, &QPushButton::clicked, this, [this, addr]() {
            tabs_->setCurrentIndex(1);
            log_stack_->setCurrentWidget(peer_logs_[addr]);
        });
        peer_rows_layout_->insertWidget(peer_rows_layout_->count() - 1, row);
        peer_stack_->setCurrentIndex(1);
        peer_input_->clear();
        add_row_->setVisible(false);
        updateStatus();

        spinner_->setVisible(true);
        status_dot_->setStyleSheet("color: #b0b8c8; font-size: 9px;");
        own_address_->setText("—");
        controller_->start(QStringList(peer_addresses_.begin(), peer_addresses_.end()));
    });

    connect(copy_btn_, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(own_address_->text());
    });
}

void MainWindow::closeEvent(QCloseEvent* event) {
    controller_->stop();
    event->accept();
}

void MainWindow::updateStatus() {
    const int total = peer_addresses_.size();
    if (total == 0) {
        status_label_->setText("No peers");
    } else {
        status_label_->setText(QString("%1 peer%2 added").arg(total).arg(total > 1 ? "s" : ""));
    }
}
