#pragma once
#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QLineEdit>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    QLabel* address_label_;
    QLineEdit* peer_input_;
    QPushButton* connect_btn_;
    QTextEdit* log_area_;
};
