#include <QApplication>
#include <oclero/qlementine.hpp>
#include "main_window.hpp"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    auto* style = new oclero::qlementine::QlementineStyle(&app);
    style->setThemeJsonPath(":/resources/theme.json");
    QApplication::setStyle(style);
    MainWindow window;
    window.show();
    return app.exec();
}
