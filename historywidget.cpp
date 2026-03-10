#include "historywidget.h"
#include "databasemanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QDateEdit>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTabWidget>
#include <QGroupBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringConverter>
#endif
#include <QMessageBox>
#include <QDate>

HistoryWidget::HistoryWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
    loadData();
}

void HistoryWidget::setupUI() {
    QVBoxLayout* vl = new QVBoxLayout(this);
    vl->setContentsMargins(12,12,12,12);

    QLabel* title = new QLabel("历史操作与数据查询", this);
    title->setStyleSheet("font-size:16px;font-weight:bold;color:#2c3e50;");
    vl->addWidget(title);

    // 筛选条件
    QGroupBox* filterBox = new QGroupBox("查询条件", this);
    QHBoxLayout* filterLay = new QHBoxLayout(filterBox);

    filterLay->addWidget(new QLabel("开始日期:"));
    m_startDate = new QDateEdit(QDate::currentDate().addMonths(-1), filterBox);
    m_startDate->setCalendarPopup(true);
    m_startDate->setDisplayFormat("yyyy-MM-dd");
    filterLay->addWidget(m_startDate);

    filterLay->addWidget(new QLabel("结束日期:"));
    m_endDate = new QDateEdit(QDate::currentDate(), filterBox);
    m_endDate->setCalendarPopup(true);
    m_endDate->setDisplayFormat("yyyy-MM-dd");
    filterLay->addWidget(m_endDate);

    filterLay->addWidget(new QLabel("设备类型:"));
    m_typeFilter = new QComboBox(filterBox);
    m_typeFilter->addItems({"全部","灯光","空调","窗帘","摄像头","传感器"});
    filterLay->addWidget(m_typeFilter);

    QPushButton* queryBtn = new QPushButton("🔍 查询", filterBox);
    queryBtn->setStyleSheet("background:#3498db;color:white;padding:5px 14px;border-radius:4px;");
    connect(queryBtn, &QPushButton::clicked, this, &HistoryWidget::onQuery);
    filterLay->addWidget(queryBtn);

    QPushButton* exportBtn = new QPushButton("📤 导出CSV", filterBox);
    exportBtn->setStyleSheet("background:#27ae60;color:white;padding:5px 14px;border-radius:4px;");
    connect(exportBtn, &QPushButton::clicked, this, &HistoryWidget::onExport);
    filterLay->addWidget(exportBtn);
    filterLay->addStretch();
    vl->addWidget(filterBox);

    // 标签页
    m_tabs = new QTabWidget(this);

    // 操作记录表
    m_table = new QTableWidget(0, 6, this);
    m_table->setHorizontalHeaderLabels({"ID","操作人","设备名称","操作内容","结果","时间"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tabs->addTab(m_table, "操作记录");

    // 环境数据表
    m_envTable = new QTableWidget(0, 4, this);
    m_envTable->setHorizontalHeaderLabels({"温度(℃)","湿度(%)","空气质量","记录时间"});
    m_envTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_envTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_envTable->setAlternatingRowColors(true);
    m_tabs->addTab(m_envTable, "环境数据");

    vl->addWidget(m_tabs);
}

void HistoryWidget::loadData() {
    // 加载操作记录
    QString typeFilter = m_typeFilter ? (m_typeFilter->currentIndex() == 0 ? "" : m_typeFilter->currentText()) : "";
    QString startStr = m_startDate ? m_startDate->date().toString("yyyy-MM-dd") : "";
    QString endStr   = m_endDate   ? m_endDate->date().toString("yyyy-MM-dd")   : "";

    auto logs = DatabaseManager::instance()->getOperationLogs(typeFilter, startStr, endStr);
    m_table->setRowCount(0);
    for (const auto& log : logs) {
        int row = m_table->rowCount();
        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(log["id"].toString()));
        m_table->setItem(row, 1, new QTableWidgetItem(log["username"].toString()));
        m_table->setItem(row, 2, new QTableWidgetItem(log["device_name"].toString()));
        m_table->setItem(row, 3, new QTableWidgetItem(log["action"].toString()));
        QString res = log["result"].toString();
        QTableWidgetItem* resItem = new QTableWidgetItem(res);
        resItem->setForeground(res == "success" ? Qt::darkGreen : Qt::red);
        m_table->setItem(row, 4, resItem);
        m_table->setItem(row, 5, new QTableWidgetItem(log["created_at"].toString()));
    }

    // 加载环境数据
    auto envData = DatabaseManager::instance()->getEnvData(startStr, endStr);
    m_envTable->setRowCount(0);
    for (const auto& d : envData) {
        int row = m_envTable->rowCount();
        m_envTable->insertRow(row);
        m_envTable->setItem(row, 0, new QTableWidgetItem(QString::number(d["temperature"].toDouble(),'f',1)));
        m_envTable->setItem(row, 1, new QTableWidgetItem(QString::number(d["humidity"].toDouble(),'f',1)));
        m_envTable->setItem(row, 2, new QTableWidgetItem(QString::number(d["air_quality"].toDouble(),'f',1)));
        m_envTable->setItem(row, 3, new QTableWidgetItem(d["created_at"].toString()));
    }
}

void HistoryWidget::onQuery() {
    loadData();
}

void HistoryWidget::onExport() {
    QString path = QFileDialog::getSaveFileName(this, "导出CSV", "operation_log.csv",
                                                "CSV文件 (*.csv)");
    if (path.isEmpty()) return;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "错误", "无法打开文件写入"); return;
    }
    QTextStream ts(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    ts.setEncoding(QStringConverter::Utf8);
#else
    ts.setCodec("UTF-8");
#endif
    // 写表头
    QStringList headers;
    for (int c = 0; c < m_table->columnCount(); c++)
        headers << m_table->horizontalHeaderItem(c)->text();
    ts << headers.join(",") << "\n";
    // 写数据
    for (int r = 0; r < m_table->rowCount(); r++) {
        QStringList row;
        for (int c = 0; c < m_table->columnCount(); c++)
            row << (m_table->item(r,c) ? m_table->item(r,c)->text() : "");
        ts << row.join(",") << "\n";
    }
    file.close();
    QMessageBox::information(this, "成功", QString("已导出 %1 条记录到:\n%2").arg(m_table->rowCount()).arg(path));
}
