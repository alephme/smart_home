#include "settingswidget.h"
#include "databasemanager.h"

#include <QApplication>      // 新增：用于全局样式设置
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>
#include <QFileDialog>
SettingsWidget::SettingsWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
    // 启动时应用当前保存的主题
    applyTheme(m_themeCombo->currentText());
}

void SettingsWidget::setupUI() {
    QVBoxLayout* vl = new QVBoxLayout(this);
    vl->setContentsMargins(12, 12, 12, 12);

    QLabel* title = new QLabel("系统设置", this);
    title->setStyleSheet("font-size:16px;font-weight:bold;color:#2c3e50;");
    vl->addWidget(title);

    QWidget* panel = new QWidget(this);
    QFormLayout* sysForm = new QFormLayout(panel);
    sysForm->setSpacing(14);
    sysForm->setContentsMargins(20, 20, 20, 20);

    m_themeCombo = new QComboBox(panel);
    m_themeCombo->addItem("默认主题");
    m_themeCombo->addItem("深色主题");
    m_themeCombo->addItem("浅蓝主题");
    m_themeCombo->setCurrentText(DatabaseManager::instance()->getSetting("theme", "默认主题"));
    sysForm->addRow("界面主题:", m_themeCombo);

    m_refreshSpin = new QSpinBox(panel);
    m_refreshSpin->setRange(1, 60);
    m_refreshSpin->setSuffix(" 秒");
    m_refreshSpin->setValue(DatabaseManager::instance()->getSetting("refresh_interval", "5").toInt());
    sysForm->addRow("数据刷新间隔:", m_refreshSpin);

    QHBoxLayout* dbPathLayout = new QHBoxLayout;
    m_dbPathEdit = new QLineEdit(DatabaseManager::instance()->getSetting("db_path", "smart_home.db"), panel);
    dbPathLayout->addWidget(m_dbPathEdit);
    QPushButton* browseBtn = new QPushButton("浏览", panel);
    connect(browseBtn, &QPushButton::clicked, [=]() {
        QString filePath = QFileDialog::getOpenFileName(this, "选择数据库文件", "", "SQLite数据库 (*.db)");
        if (!filePath.isEmpty()) {
            m_dbPathEdit->setText(filePath);
        }
    });
    dbPathLayout->addWidget(browseBtn);
    sysForm->addRow("数据库路径:", dbPathLayout);
    QPushButton* saveBtn = new QPushButton("保存设置", panel);
    saveBtn->setStyleSheet("background:#2c3e50;color:white;padding:8px 20px;border-radius:4px;font-size:14px;");
    connect(saveBtn, &QPushButton::clicked, this, &SettingsWidget::onSaveSettings);
    sysForm->addRow(saveBtn);

    vl->addWidget(panel);
    vl->addStretch();
}

void SettingsWidget::onSaveSettings() {
    DatabaseManager::instance()->setSetting("theme", m_themeCombo->currentText());
    DatabaseManager::instance()->setSetting("refresh_interval", QString::number(m_refreshSpin->value()));
    DatabaseManager::instance()->setSetting("db_path", m_dbPathEdit->text());

    applyTheme(m_themeCombo->currentText());

    // 提示重启生效
    QMessageBox::information(this, "成功", "设置已保存，修改数据库路径需要重启程序才能生效。");
}

// 新增：根据主题名称设置全局样式表
void SettingsWidget::applyTheme(const QString& themeName) {
    if (themeName == "深色主题") {
        qApp->setStyleSheet(
            "QWidget { background-color: #2b2b2b; color: #f0f0f0; }"
            "QPushButton { background-color: #3c3c3c; border: 1px solid #555; padding: 5px; }"
            "QPushButton:hover { background-color: #4a4a4a; }"
            "QLineEdit, QComboBox, QSpinBox { background-color: #3c3c3c; color: #f0f0f0; border: 1px solid #555; }"
            "QHeaderView::section { background-color: #3c3c3c; color: #f0f0f0; }" // 表格头部
            "QTabWidget::pane { background-color: #2b2b2b; }"
            "QTabBar::tab { background-color: #3c3c3c; color: #f0f0f0; padding: 6px; }"
            "QTabBar::tab:selected { background-color: #4a4a4a; }"
            );
    } else if (themeName == "浅蓝主题") {
        qApp->setStyleSheet(
            "QWidget { background-color: #e6f0fa; color: #1e3a5f; }"
            "QPushButton { background-color: #3a6ea5; color: white; border: none; padding: 5px; }"
            "QPushButton:hover { background-color: #2a4f7a; }"
            "QLineEdit, QComboBox, QSpinBox { background-color: white; color: black; border: 1px solid #3a6ea5; }"
            "QHeaderView::section { background-color: #3a6ea5; color: white; }"
            "QTabWidget::pane { background-color: #e6f0fa; }"
            "QTabBar::tab { background-color: #b0c4de; color: #1e3a5f; padding: 6px; }"
            "QTabBar::tab:selected { background-color: #e6f0fa; }"
            );
    } else { // 默认主题
        qApp->setStyleSheet(""); // 清空样式表，恢复系统默认
    }
}
