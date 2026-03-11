#include <QApplication>
#include <QFont>
#include <QFile>
#include <QIcon>
#include <QStyle>
#include "mainwindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("智能家居监控平台");
    app.setApplicationVersion("1.0");

    QIcon appIcon;
    if (QFile::exists(":/icons/app.ico")) {
        appIcon = QIcon(":/icons/app.ico");
    }
    if (appIcon.isNull()) {
        appIcon = app.style()->standardIcon(QStyle::SP_ComputerIcon);
    }
    app.setWindowIcon(appIcon);

    // 全局字体
    QFont font;
    font.setFamily("Microsoft YaHei");
    font.setPixelSize(13);
    app.setFont(font);

    // 全局样式
    app.setStyleSheet(R"(
        QWidget { font-family: 'Microsoft YaHei', 'Arial', sans-serif; }
        QGroupBox { font-weight: bold; border: 1px solid #bdc3c7; border-radius: 6px; margin-top: 8px; padding-top: 8px; }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }
        QPushButton { border-radius: 4px; }
        QLineEdit, QComboBox, QSpinBox, QDateEdit {
            border: 1px solid #bdc3c7; border-radius: 4px; padding: 4px 6px;
        }
        QLineEdit:focus, QComboBox:focus { border-color: #3498db; }
        QTableWidget { gridline-color: #ecf0f1; }
        QTableWidget QHeaderView::section {
            background: #2c3e50; color: white; padding: 6px; border: none;
        }
        QTabWidget::pane { border: 1px solid #bdc3c7; }
        QTabBar::tab { padding: 8px 16px; background: #ecf0f1; border: 1px solid #bdc3c7; border-bottom: none; }
        QTabBar::tab:selected { background: white; }
    )");

    MainWindow w;
    w.setWindowIcon(appIcon);
    w.show();
    return app.exec();
}
