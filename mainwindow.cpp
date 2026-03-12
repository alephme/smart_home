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
#include <QScrollArea>
#include <QResizeEvent>
#include <QFont>
#include <QMap>
#include <QRandomGenerator>
#include <QStringList>
#include <QtGlobal>

namespace {
// 解析设备参数字符串（k1=v1;k2=v2）为键值映射。
QMap<QString, QString> parseParams(const QString& params) {
    QMap<QString, QString> kv;
    const QStringList pairs = params.split(';', Qt::SkipEmptyParts);
    for (const QString& p : pairs) {
        const int idx = p.indexOf('=');
        if (idx <= 0) {
            continue;
        }
        kv[p.left(idx).trimmed()] = p.mid(idx + 1).trimmed();
    }
    return kv;
}

double randomInRange(double minValue, double maxValue) {
    const double ratio = QRandomGenerator::global()->generateDouble();
    return minValue + (maxValue - minValue) * ratio;
}

double randomAround(double center, double spread, double minValue, double maxValue) {
    return qBound(minValue, center + randomInRange(-spread, spread), maxValue);
}
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("智能家居监控平台");
    // setWindowIcon(QIcon(":/icons/smarthome.ico"));
    resize(820, 520);
    setMinimumSize(480, 320);

    // 初始化数据库
    DatabaseManager::instance()->init();

    // 先显示登录界面（覆盖整个窗口）
    m_loginWidget = new LoginWidget(this);
    QScrollArea* loginScroll = new QScrollArea(this);
    loginScroll->setWidget(m_loginWidget);
    loginScroll->setWidgetResizable(true);
    loginScroll->setFrameShape(QFrame::NoFrame);
    setCentralWidget(loginScroll);
    connect(m_loginWidget, &LoginWidget::loginSuccess, this, &MainWindow::onLoginSuccess);

    applyAdaptiveUiScale();
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

    applyAdaptiveUiScale();
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    applyAdaptiveUiScale();
}

void MainWindow::applyAdaptiveUiScale() {
    // Keep scaling gentle so the UI remains readable while still responsive.
    const double sx = width() / 1200.0;
    const double sy = height() / 750.0;
    const double scale = qBound(0.80, qMin(sx, sy), 1.25);

    const int fontPt = qBound(9, static_cast<int>(10 * scale), 13);
    if (fontPt != m_lastFontPt) {
        QFont f = qApp->font();
        f.setPointSize(fontPt);
        qApp->setFont(f);
        m_lastFontPt = fontPt;
    }

    if (m_navBar) {
        const int navH = qBound(40, static_cast<int>(50 * scale), 64);
        m_navBar->setFixedHeight(navH);
        updateNavButtonStyles(scale);

        if (m_logoLabel) {
            QFont lf = m_logoLabel->font();
            lf.setPointSize(qBound(11, static_cast<int>(14 * scale), 18));
            lf.setBold(true);
            m_logoLabel->setFont(lf);
        }

        if (m_userLabel) {
            QFont uf = m_userLabel->font();
            uf.setPointSize(qBound(9, static_cast<int>(10 * scale), 13));
            m_userLabel->setFont(uf);
        }
    }
}

void MainWindow::updateNavButtonStyles(double scale) {
    const int padV = qBound(4, static_cast<int>(6 * scale), 10);
    const int padH = qBound(8, static_cast<int>(14 * scale), 18);
    const int radius = qBound(3, static_cast<int>(5 * scale), 8);
    const int fontPt = qBound(9, static_cast<int>(10 * scale), 13);

    for (int i = 0; i < m_navButtons.size(); ++i) {
        QPushButton* btn = m_navButtons.at(i);
        if (!btn) {
            continue;
        }
        const bool active = (i == m_currentPage);
        const QString bg = active ? "#1abc9c" : "transparent";
        const QString hover = active ? "#17a589" : "#34495e";
        btn->setStyleSheet(QString(
            "QPushButton{color:white;padding:%1px %2px;border-radius:%3px;"
            "background:%4;font-weight:%5;font-size:%6pt;}"
            "QPushButton:hover{background:%7;}"
        ).arg(padV).arg(padH).arg(radius).arg(bg).arg(active ? 700 : 500).arg(fontPt).arg(hover));
    }
}

void MainWindow::setupUI() {
    m_homeWidget    = new HomeWidget(m_currentUser, this);
    m_deviceWidget  = new DeviceControlWidget(m_currentUser, this);
    m_sceneWidget   = new SceneWidget(m_currentUser, this);
    m_historyWidget = new HistoryWidget(this);
    m_alarmWidget   = new AlarmWidget(this);
    m_settingsWidget= new SettingsWidget(this);

    m_stack = new QStackedWidget(this);

    auto wrapScrollable = [this](QWidget* page) {
        QScrollArea* scroll = new QScrollArea(this);
        scroll->setWidget(page);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        return scroll;
    };

    m_stack->addWidget(wrapScrollable(m_homeWidget));
    m_stack->addWidget(wrapScrollable(m_deviceWidget));
    m_stack->addWidget(wrapScrollable(m_sceneWidget));
    m_stack->addWidget(wrapScrollable(m_historyWidget));
    m_stack->addWidget(wrapScrollable(m_alarmWidget));
    m_stack->addWidget(wrapScrollable(m_settingsWidget));

    // 页面间信号连接
    connect(m_homeWidget, &HomeWidget::navigateTo, m_stack, &QStackedWidget::setCurrentIndex);
    connect(m_deviceWidget, &DeviceControlWidget::deviceChanged, m_homeWidget, &HomeWidget::refresh);
    connect(m_alarmWidget, &AlarmWidget::alarmTriggered, [this](const QString& msg){
        m_alarmWidget->addAlarm("设备异常", msg, "系统");
    });
    connect(m_settingsWidget, &SettingsWidget::samplingIntervalChanged,
            this, &MainWindow::applyEnvSamplingInterval);

    setupEnvSampling();
}

void MainWindow::setupEnvSampling() {
    // 启动全局环境采样定时器，间隔来自系统设置。
    if (!m_envSampleTimer) {
        m_envSampleTimer = new QTimer(this);
        connect(m_envSampleTimer, &QTimer::timeout, this, &MainWindow::onEnvSamplingTick);
    }

    const int seconds = DatabaseManager::instance()
                            ->getSetting("sampling_interval",
                                         DatabaseManager::instance()->getSetting("refresh_interval", "10"))
                            .toInt();
    applyEnvSamplingInterval(seconds);
}

void MainWindow::applyEnvSamplingInterval(int seconds) {
    const int safeSeconds = qBound(1, seconds, 3600);
    if (!m_envSampleTimer) {
        return;
    }

    m_envSampleTimer->setInterval(safeSeconds * 1000);
    if (!m_envSampleTimer->isActive()) {
        m_envSampleTimer->start();
    }
}

void MainWindow::onEnvSamplingTick() {
    // 定时生成一条环境采样并写入数据库，供历史/报警模块读取。
    auto envData = DatabaseManager::instance()->getEnvData();
    double temperature = 25.0;
    double humidity = 55.0;
    double airQuality = 32.0;
    if (!envData.isEmpty()) {
        const auto& latest = envData.first();
        temperature = latest.value("temperature").toDouble();
        humidity = latest.value("humidity").toDouble();
        airQuality = latest.value("air_quality").toDouble();
    }

    bool acOnline = false;
    bool purifierOnline = false;
    int targetTemp = 24;
    QString acMode = "自动";

    const auto devices = DatabaseManager::instance()->getDevices();
    for (const auto& d : devices) {
        const QString type = d.value("type").toString();
        const QString status = d.value("status").toString();
        if (status != "online") {
            continue;
        }

        const QMap<QString, QString> kv = parseParams(d.value("params").toString());
        const bool powerOn = (kv.value("power", "on") != "off");
        if (!powerOn) {
            continue;
        }

        if (type == "空调" && !acOnline) {
            acOnline = true;
            targetTemp = kv.value("temp", "24").toInt();
            acMode = kv.value("mode", "自动");
        }
        if (type == "空气净化器") {
            purifierOnline = true;
        }
    }

    if (acOnline) {
        if (acMode == "制冷") {
            temperature = randomAround(targetTemp + 0.8, 0.5, targetTemp - 0.4, targetTemp + 2.0);
            humidity = randomAround(48.0, 2.5, 38.0, 60.0);
        } else if (acMode == "制热") {
            temperature = randomAround(targetTemp - 0.5, 0.6, targetTemp - 1.8, targetTemp + 1.2);
            humidity = randomAround(42.0, 2.5, 30.0, 55.0);
        } else if (acMode == "除湿") {
            temperature = randomAround(targetTemp + 0.2, 0.6, targetTemp - 1.0, targetTemp + 1.2);
            humidity = randomAround(38.0, 2.2, 28.0, 50.0);
        } else {
            temperature = randomAround(targetTemp, 0.7, 21.0, 29.0);
            humidity = randomAround(50.0, 3.0, 36.0, 64.0);
        }
    } else {
        temperature = randomAround(temperature, 0.4, 20.0, 32.0);
        humidity = randomAround(humidity, 1.2, 30.0, 80.0);
    }

    if (purifierOnline) {
        airQuality = randomAround(airQuality - 1.2, 1.5, 8.0, 80.0);
    } else {
        airQuality = randomAround(airQuality + 0.6, 1.8, 10.0, 120.0);
    }

    DatabaseManager::instance()->addEnvData(temperature, humidity, airQuality);
}

void MainWindow::setupNavBar() {
    // 顶部导航栏负责页面切换与用户会话操作。
    m_navBar = new QWidget(this);
    m_navBar->setFixedHeight(50);
    m_navBar->setStyleSheet("background:#2c3e50; color:white;");

    QHBoxLayout* hl = new QHBoxLayout(m_navBar);
    hl->setContentsMargins(10,0,10,0);

    m_logoLabel = new QLabel("🏠 智能家居监控平台", m_navBar);
    m_logoLabel->setStyleSheet("color:white; font-size:16px; font-weight:bold;");
    hl->addWidget(m_logoLabel);
    hl->addStretch();

    m_navButtons.clear();

    auto makeBtn = [&](const QString& text, int page) {
        QPushButton* btn = new QPushButton(text, m_navBar);
        btn->setFlat(true);
        m_navButtons.append(btn);
        connect(btn, &QPushButton::clicked, [this, page]{
            m_currentPage = page;
            m_stack->setCurrentIndex(page);
            applyAdaptiveUiScale();
        });
        hl->addWidget(btn);
    };

    makeBtn("首页",       PAGE_HOME);
    makeBtn("设备控制",   PAGE_DEVICE);
    makeBtn("场景模式",   PAGE_SCENE);
    makeBtn("历史记录",   PAGE_HISTORY);
    makeBtn("报警管理",   PAGE_ALARM);
    makeBtn("系统设置",   PAGE_SETTINGS);

    connect(m_stack, &QStackedWidget::currentChanged, this, [this](int index) {
        m_currentPage = index;
        applyAdaptiveUiScale();
    });

    m_userLabel = new QLabel("用户: " + m_currentUser, m_navBar);
    m_userLabel->setStyleSheet("color:#bdc3c7; margin-left:10px;");
    hl->addWidget(m_userLabel);

    QPushButton* logoutBtn = new QPushButton("退出", m_navBar);
    logoutBtn->setStyleSheet("QPushButton{color:#e74c3c;padding:4px 10px;border:1px solid #e74c3c;border-radius:4px;}"
                             "QPushButton:hover{background:#e74c3c;color:white;}");
    connect(logoutBtn, &QPushButton::clicked, this, &MainWindow::onLogout);
    hl->addWidget(logoutBtn);

    applyAdaptiveUiScale();
}

void MainWindow::onLogout() {
    m_loginWidget = new LoginWidget(this);
    connect(m_loginWidget, &LoginWidget::loginSuccess, this, &MainWindow::onLoginSuccess);
    QScrollArea* loginScroll = new QScrollArea(this);
    loginScroll->setWidget(m_loginWidget);
    loginScroll->setWidgetResizable(true);
    loginScroll->setFrameShape(QFrame::NoFrame);
    setCentralWidget(loginScroll);
    m_currentUser.clear();
}
