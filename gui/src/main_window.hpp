#pragma once
#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QSet>
#include <QMap>
#include <QString>
#include <QCloseEvent>
#include <oclero/qlementine/widgets/LoadingSpinner.hpp>
#include <oclero/qlementine/widgets/LineEdit.hpp>
#include <oclero/qlementine/widgets/Label.hpp>
#include <oclero/qlementine/widgets/SegmentedControl.hpp>
#include "switch_controller.hpp"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    oclero::qlementine::SegmentedControl* tabs_;
    QStackedWidget* pages_;

    QLabel* status_dot_;
    QLabel* status_label_;
    oclero::qlementine::LoadingSpinner* spinner_;
    QLabel* own_address_;
    QLabel* tap_address_;
    QPushButton* copy_btn_;
    QPushButton* tunnelblick_btn_;
    QWidget* add_row_;
    oclero::qlementine::LineEdit* peer_input_;
    QStackedWidget* peer_stack_;
    QWidget* peer_rows_;
    QVBoxLayout* peer_rows_layout_;

    QSet<QString> peer_addresses_;

    QMap<QString, QTextEdit*> peer_logs_;
    QStackedWidget* log_stack_;

    SwitchController* controller_;

    void updateStatus();
};
