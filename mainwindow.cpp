#include "mainwindow.h"
#include "loginwidget.h"
#include "homewidget.h"
#include "devicecontrolwidget.h"
#include "scenewidget.h"
#include "historywidget.h"
#include "alarmwidget.h"
#include "settingswidget.h"
#include "databasemanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QApplication>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("智能家居监控平台");
    // setWindowIcon(QIcon(":/icons/smarthome.ico"));
    resize(1200, 750);
    setMinimumSize(900, 600);

    // 初始化数据库
    DatabaseManager::instance()->init();

    // 先显示登录界面（覆盖整个窗口）
    m_loginWidget = new LoginWidget(this);
    setCentralWidget(m_loginWidget);
    connect(m_loginWidget, &LoginWidget::loginSuccess, this, &MainWindow::onLoginSuccess);
}

void MainWindow::onLoginSuccess(const QString& username) {
    m_currentUser = username;
    setupUI();
    setupNavBar();

    // 用主界面替换登录界面
    QWidget* main = new QWidget(this);
    QVBoxLayout* vl = new QVBoxLayout(main);
    vl->setContentsMargins(0,0,0,0);
    vl->setSpacing(0);
    vl->addWidget(m_navBar);
    vl->addWidget(m_stack);
    setCentralWidget(main);
    m_mainWidget = main;
}

void MainWindow::setupUI() {
    m_homeWidget    = new HomeWidget(m_currentUser, this);
    m_deviceWidget  = new DeviceControlWidget(m_currentUser, this);
    m_sceneWidget   = new SceneWidget(m_currentUser, this);
    m_historyWidget = new HistoryWidget(this);
    m_alarmWidget   = new AlarmWidget(this);
    m_settingsWidget= new SettingsWidget(this);

    m_stack = new QStackedWidget(this);
    m_stack->addWidget(m_homeWidget);
    m_stack->addWidget(m_deviceWidget);
    m_stack->addWidget(m_sceneWidget);
    m_stack->addWidget(m_historyWidget);
    m_stack->addWidget(m_alarmWidget);
    m_stack->addWidget(m_settingsWidget);

    // 页面间信号连接
    connect(m_homeWidget, &HomeWidget::navigateTo, m_stack, &QStackedWidget::setCurrentIndex);
    connect(m_deviceWidget, &DeviceControlWidget::deviceChanged, m_homeWidget, &HomeWidget::refresh);
    connect(m_alarmWidget, &AlarmWidget::alarmTriggered, [this](const QString& msg){
        m_alarmWidget->addAlarm("设备异常", msg, "系统");
    });
}

void MainWindow::setupNavBar() {
    m_navBar = new QWidget(this);
    m_navBar->setFixedHeight(50);
    m_navBar->setStyleSheet("background:#2c3e50; color:white;");

    QHBoxLayout* hl = new QHBoxLayout(m_navBar);
    hl->setContentsMargins(10,0,10,0);

    QLabel* logo = new QLabel("🏠 智能家居监控平台", m_navBar);
    logo->setStyleSheet("color:white; font-size:16px; font-weight:bold;");
    hl->addWidget(logo);
    hl->addStretch();

    auto makeBtn = [&](const QString& text, int page) {
        QPushButton* btn = new QPushButton(text, m_navBar);
        btn->setFlat(true);
        btn->setStyleSheet("QPushButton{color:white;padding:6px 14px;border-radius:4px;}"
                           "QPushButton:hover{background:#34495e;}");
        connect(btn, &QPushButton::clicked, [this, page]{ m_stack->setCurrentIndex(page); });
        hl->addWidget(btn);
    };

    makeBtn("首页",       PAGE_HOME);
    makeBtn("设备控制",   PAGE_DEVICE);
    makeBtn("场景模式",   PAGE_SCENE);
    makeBtn("历史记录",   PAGE_HISTORY);
    makeBtn("报警管理",   PAGE_ALARM);
    makeBtn("系统设置",   PAGE_SETTINGS);

    m_userLabel = new QLabel("用户: " + m_currentUser, m_navBar);
    m_userLabel->setStyleSheet("color:#bdc3c7; margin-left:10px;");
    hl->addWidget(m_userLabel);

    QPushButton* logoutBtn = new QPushButton("退出", m_navBar);
    logoutBtn->setStyleSheet("QPushButton{color:#e74c3c;padding:4px 10px;border:1px solid #e74c3c;border-radius:4px;}"
                             "QPushButton:hover{background:#e74c3c;color:white;}");
    connect(logoutBtn, &QPushButton::clicked, this, &MainWindow::onLogout);
    hl->addWidget(logoutBtn);
}

void MainWindow::onLogout() {
    m_loginWidget = new LoginWidget(this);
    connect(m_loginWidget, &LoginWidget::loginSuccess, this, &MainWindow::onLoginSuccess);
    setCentralWidget(m_loginWidget);
    m_currentUser.clear();
}
