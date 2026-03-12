#include "alarmwidget.h"
#include "databasemanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QGroupBox>
#include <QSpinBox>
#include <QTimer>
#include <QMessageBox>

AlarmWidget::AlarmWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
    loadAlarms();
    m_checkTimer = new QTimer(this);
    connect(m_checkTimer, &QTimer::timeout, this, &AlarmWidget::onCheckAlarms);
    m_checkTimer->start(10000); // 每10秒检查一次
}

void AlarmWidget::setupUI() {
    QVBoxLayout* vl = new QVBoxLayout(this);
    vl->setContentsMargins(12,12,12,12);

    QLabel* title = new QLabel("异常报警管理", this);
    title->setStyleSheet("font-size:16px;font-weight:bold;color:#2c3e50;");
    vl->addWidget(title);

    // 报警阈值设置
    QGroupBox* threshBox = new QGroupBox("报警阈值设置", this);
    QHBoxLayout* threshLay = new QHBoxLayout(threshBox);
    threshLay->addWidget(new QLabel("温度阈值(℃):"));
    m_tempThresh = new QSpinBox(threshBox);
    m_tempThresh->setRange(20, 60); m_tempThresh->setValue(35);
    threshLay->addWidget(m_tempThresh);
    threshLay->addWidget(new QLabel("湿度阈值(%):"));
    m_humidThresh = new QSpinBox(threshBox);
    m_humidThresh->setRange(50, 100); m_humidThresh->setValue(80);
    threshLay->addWidget(m_humidThresh);

    QPushButton* manualCheckBtn = new QPushButton("立即检查", threshBox);
    manualCheckBtn->setStyleSheet(
        "QPushButton{background:#e67e22;color:white;padding:5px 12px;border-radius:4px;}"
        "QPushButton:hover{background:#f39c12;}"
        "QPushButton:pressed{background:#b85e1a;}"
        );
    connect(manualCheckBtn, &QPushButton::clicked, this, &AlarmWidget::onCheckAlarms);
    threshLay->addWidget(manualCheckBtn);
    threshLay->addStretch();
    vl->addWidget(threshBox);

    // 状态栏
    m_statusLbl = new QLabel("系统正常运行中...", this);
    m_statusLbl->setStyleSheet("color:#27ae60;font-size:13px;padding:4px;background:#eafaf1;border-radius:4px;");
    vl->addWidget(m_statusLbl);

    // 报警记录表
    QGroupBox* tableBox = new QGroupBox("报警记录", this);
    QVBoxLayout* tableVl = new QVBoxLayout(tableBox);

    m_table = new QTableWidget(0, 5, this);
    m_table->setHorizontalHeaderLabels({"ID","报警类型","报警内容","相关设备","报警时间"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableVl->addWidget(m_table);

    QHBoxLayout* btnRow = new QHBoxLayout();
    QPushButton* refreshBtn = new QPushButton("🔄 刷新", tableBox);
    refreshBtn->setStyleSheet(
        "QPushButton{background:#3498db;color:white;padding:5px 12px;border-radius:4px;}"
        "QPushButton:hover{background:#5dade2;}"
        "QPushButton:pressed{background:#1f6a9a;}"
        );
    connect(refreshBtn, &QPushButton::clicked, this, &AlarmWidget::loadAlarms);

    QPushButton* deleteBtn = new QPushButton("🗑 删除选中", tableBox);
    deleteBtn->setStyleSheet("background:#e74c3c;color:white;padding:5px 12px;border-radius:4px;");
    connect(deleteBtn, &QPushButton::clicked, this, &AlarmWidget::onDeleteAlarm);

    QPushButton* clearBtn = new QPushButton("🗑 清空全部", tableBox);
    clearBtn->setStyleSheet("background:#7f8c8d;color:white;padding:5px 12px;border-radius:4px;");
    connect(clearBtn, &QPushButton::clicked, this, &AlarmWidget::onClearAlarms);

    btnRow->addWidget(refreshBtn);
    btnRow->addWidget(deleteBtn);
    btnRow->addWidget(clearBtn);
    btnRow->addStretch();
    tableVl->addLayout(btnRow);
    vl->addWidget(tableBox);
}

void AlarmWidget::loadAlarms() {
    auto alarms = DatabaseManager::instance()->getAlarms();
    m_table->setRowCount(0);
    int unread = 0;
    for (const auto& a : alarms) {
        int row = m_table->rowCount();
        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(a["id"].toString()));
        QTableWidgetItem* typeItem = new QTableWidgetItem(a["type"].toString());
        typeItem->setForeground(Qt::red);
        m_table->setItem(row, 1, typeItem);
        m_table->setItem(row, 2, new QTableWidgetItem(a["content"].toString()));
        m_table->setItem(row, 3, new QTableWidgetItem(a["device_name"].toString()));
        m_table->setItem(row, 4, new QTableWidgetItem(a["created_at"].toString()));
        if (!a["is_read"].toBool()) unread++;
    }
    if (alarms.isEmpty()) {
        m_statusLbl->setText("✅ 系统正常，无报警记录");
        m_statusLbl->setStyleSheet("color:#27ae60;font-size:13px;padding:4px;background:#eafaf1;border-radius:4px;");
    } else {
        m_statusLbl->setText(QString("⚠️ 共 %1 条报警，%2 条未读").arg(alarms.size()).arg(unread));
        m_statusLbl->setStyleSheet("color:#e74c3c;font-size:13px;padding:4px;background:#fdecea;border-radius:4px;");
    }
}

void AlarmWidget::addAlarm(const QString& type, const QString& content, const QString& deviceName) {
    DatabaseManager::instance()->addAlarm(type, content, deviceName);
    loadAlarms();
    // 弹出报警提示
    QMessageBox::warning(this, "⚠️ 系统报警", QString("[%1] %2\n设备: %3").arg(type, content, deviceName));
}

void AlarmWidget::onDeleteAlarm() {
    int row = m_table->currentRow();
    if (row < 0) { QMessageBox::information(this,"提示","请先选择一条报警记录"); return; }
    int id = m_table->item(row, 0)->text().toInt();
    DatabaseManager::instance()->deleteAlarm(id);
    loadAlarms();
}

void AlarmWidget::onClearAlarms() {
    if (QMessageBox::question(this,"确认","确定要清空所有报警记录吗？") == QMessageBox::Yes) {
        DatabaseManager::instance()->clearAlarms();
        loadAlarms();
    }
}

void AlarmWidget::onCheckAlarms() {
    checkThresholds();
}

void AlarmWidget::checkThresholds() {
    // 从数据库获取最新的环境数据列表（假设按时间倒序，第一条为最新）
    auto envData = DatabaseManager::instance()->getEnvData();

    // 如果有环境数据，则检查最新一条是否超阈值
    if (!envData.isEmpty()) {
        // 获取最新一条环境数据（列表的第一个元素）
        const auto& latest = envData.first();

        // 从数据中提取温度值
        double temp = latest["temperature"].toDouble();
        // 从数据中提取湿度值
        double humid = latest["humidity"].toDouble();

        // 如果当前温度超过设定的温度阈值
        if (temp > m_tempThresh->value()) {
            // --- 防重复检查：与离线报警逻辑一致 ---
            // 获取所有现有报警记录
            auto alarms = DatabaseManager::instance()->getAlarms();
            bool alreadyAlarmed = false;
            // 遍历报警记录，查找是否已存在温度报警（设备名为"客厅空调"）
            for (const auto& a : alarms) {
                if (a["device_name"].toString() == "客厅空调" &&
                    a["type"].toString() == "温度报警") {
                    alreadyAlarmed = true;
                    break;
                }
            }
            // 如果没有现有温度报警，则添加新报警
            if (!alreadyAlarmed) {
                addAlarm("温度报警",
                         QString("当前温度 %1℃ 超过阈值 %2℃").arg(temp).arg(m_tempThresh->value()),
                         "客厅空调");
            }
        }

        // 如果当前湿度超过设定的湿度阈值
        if (humid > m_humidThresh->value()) {
            // --- 防重复检查：与离线报警逻辑一致 ---
            auto alarms = DatabaseManager::instance()->getAlarms();
            bool alreadyAlarmed = false;
            for (const auto& a : alarms) {
                if (a["device_name"].toString() == "客厅空调" &&
                    a["type"].toString() == "湿度报警") {
                    alreadyAlarmed = true;
                    break;
                }
            }
            if (!alreadyAlarmed) {
                addAlarm("湿度报警",
                         QString("当前湿度 %1% 超过阈值 %2%").arg(humid).arg(m_humidThresh->value()),
                         "客厅空调");
            }
        }
    }

    // 检查设备离线（原逻辑保持不变）
    auto devices = DatabaseManager::instance()->getDevices();
    for (const auto& d : devices) {
        if (d["status"].toString() == "offline") {
            auto alarms = DatabaseManager::instance()->getAlarms();
            bool alreadyAlarmed = false;
            for (const auto& a : alarms) {
                if (a["device_name"].toString() == d["name"].toString() &&
                    a["type"].toString() == "设备离线") {
                    alreadyAlarmed = true;
                    break;
                }
            }
            if (!alreadyAlarmed) {
                DatabaseManager::instance()->addAlarm("设备离线",
                                                      d["name"].toString() + " 当前处于离线状态",
                                                      d["name"].toString());
                loadAlarms();  // 这里直接调用了数据库接口，所以需要手动刷新表格
            }
        }
    }
}
