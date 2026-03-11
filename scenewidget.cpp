#include "scenewidget.h"
#include "databasemanager.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QSplitter>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QComboBox>
#include <QMessageBox>
#include <QScrollArea>

SceneWidget::SceneWidget(const QString& username, QWidget* parent)
    : QWidget(parent), m_username(username) {
    setupUI();
    loadScenes();
}

void SceneWidget::setupUI() {
    QVBoxLayout* vl = new QVBoxLayout(this);
    vl->setContentsMargins(12,12,12,12);

    QLabel* title = new QLabel("场景模式设置", this);
    title->setStyleSheet("font-size:16px;font-weight:bold;color:#2c3e50;");
    vl->addWidget(title);

    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);

    // 左：场景列表
    QWidget* left = new QWidget(splitter);
    QVBoxLayout* leftVl = new QVBoxLayout(left);
    leftVl->setContentsMargins(0,0,0,0);
    QLabel* listLbl = new QLabel("场景列表", left);
    listLbl->setStyleSheet("font-weight:bold;color:#555;");
    leftVl->addWidget(listLbl);

    m_sceneList = new QListWidget(left);
    m_sceneList->setStyleSheet("QListWidget::item{padding:10px;border-bottom:1px solid #ecf0f1;}"
                               "QListWidget::item:selected{background:#d6eaf8;}");
    connect(m_sceneList, &QListWidget::currentRowChanged, this, &SceneWidget::onSceneSelected);
    leftVl->addWidget(m_sceneList);

    QHBoxLayout* btnRow = new QHBoxLayout();
    auto makeBtn = [&](const QString& text, const char* slot, const QString& color) {
        QPushButton* btn = new QPushButton(text, left);
        btn->setStyleSheet(QString("QPushButton{background:%1;color:white;border-radius:4px;padding:5px;}"
                                   "QPushButton:hover{opacity:0.85;}").arg(color));
        connect(btn, SIGNAL(clicked()), this, slot);
        btnRow->addWidget(btn);
        return btn;
    };
    makeBtn("➕ 新建", SLOT(onAddScene()), "#27ae60");
    makeBtn("✏️ 编辑", SLOT(onEditScene()), "#e67e22");
    makeBtn("🗑 删除", SLOT(onDeleteScene()), "#e74c3c");
    leftVl->addLayout(btnRow);

    // 右：场景详情
    QWidget* right = new QWidget(splitter);
    QVBoxLayout* rightVl = new QVBoxLayout(right);

    QGroupBox* descBox = new QGroupBox("场景描述", right);
    QVBoxLayout* descVl = new QVBoxLayout(descBox);
    m_descLbl = new QLabel("请选择一个场景", descBox);
    m_descLbl->setWordWrap(true);
    m_descLbl->setStyleSheet("color:#555;");
    descVl->addWidget(m_descLbl);
    rightVl->addWidget(descBox);

    QGroupBox* devBox = new QGroupBox("绑定设备操作", right);
    QVBoxLayout* devVl = new QVBoxLayout(devBox);
    m_deviceList = new QListWidget(devBox);
    m_deviceList->setMaximumHeight(180);
    devVl->addWidget(m_deviceList);
    rightVl->addWidget(devBox);

    m_activateBtn = new QPushButton("▶ 激活此场景", right);
    m_activateBtn->setFixedHeight(44);
    m_activateBtn->setEnabled(false);
    m_activateBtn->setStyleSheet("QPushButton{background:#2c3e50;color:white;border-radius:6px;font-size:14px;}"
                                 "QPushButton:hover{background:#34495e;}");
    connect(m_activateBtn, &QPushButton::clicked, this, &SceneWidget::onActivateScene);
    rightVl->addWidget(m_activateBtn);
    rightVl->addStretch();

    splitter->addWidget(left);
    splitter->addWidget(right);
    splitter->setSizes({260, 500});
    vl->addWidget(splitter);
}

void SceneWidget::loadScenes() {
    m_sceneList->clear();
    auto scenes = DatabaseManager::instance()->getScenes();
    for (const auto& s : scenes) {
        QListWidgetItem* item = new QListWidgetItem("🎬 " + s["name"].toString());
        item->setData(Qt::UserRole, s["id"]);
        m_sceneList->addItem(item);
    }
}

void SceneWidget::onSceneSelected(int row) {
    if (row < 0) return;
    QListWidgetItem* item = m_sceneList->item(row);
    if (!item) return;
    m_selectedSceneId = item->data(Qt::UserRole).toInt();

    // 显示场景描述
    auto scenes = DatabaseManager::instance()->getScenes();
    for (const auto& s : scenes) {
        if (s["id"].toInt() == m_selectedSceneId) {
            m_descLbl->setText(s["description"].toString());
            break;
        }
    }

    // 显示绑定设备
    m_deviceList->clear();
    auto devs = DatabaseManager::instance()->getSceneDevices(m_selectedSceneId);
    for (const auto& d : devs) {
        m_deviceList->addItem(QString("📌 %1 → %2 %3").arg(
            d["name"].toString(), d["action"].toString(), d["params"].toString()));
    }
    m_activateBtn->setEnabled(true);
}

void SceneWidget::onAddScene() {
    QDialog dlg(this);
    dlg.setWindowTitle("新建场景");
    dlg.setFixedWidth(460);
    QVBoxLayout* vl = new QVBoxLayout(&dlg);

    QFormLayout* form = new QFormLayout();
    QLineEdit* nameEdit = new QLineEdit(&dlg);
    QLineEdit* descEdit = new QLineEdit(&dlg);
    form->addRow("场景名称:", nameEdit);
    form->addRow("场景描述:", descEdit);
    vl->addLayout(form);

    // 设备选择区域
    QGroupBox* devBox = new QGroupBox("选择绑定设备及操作", &dlg);
    QVBoxLayout* devVl = new QVBoxLayout(devBox);
    QScrollArea* scroll = new QScrollArea(&dlg);
    QWidget* scrollContent = new QWidget(scroll);
    QVBoxLayout* scrollVl = new QVBoxLayout(scrollContent);

    auto devices = DatabaseManager::instance()->getDevices();
    QList<QPair<QCheckBox*, QComboBox*>> devControls;
    for (const auto& d : devices) {
        QHBoxLayout* row = new QHBoxLayout();
        QCheckBox* cb = new QCheckBox(d["name"].toString() + " [" + d["type"].toString() + "]", scrollContent);
        cb->setProperty("deviceId", d["id"]);
        QComboBox* actionCombo = new QComboBox(scrollContent);
        actionCombo->addItems({"开启","关闭","设置参数"});
        row->addWidget(cb);
        row->addWidget(actionCombo);
        scrollVl->addLayout(row);
        devControls.append({cb, actionCombo});
    }
    scrollContent->setLayout(scrollVl);
    scroll->setWidget(scrollContent);
    scroll->setFixedHeight(200);
    devVl->addWidget(scroll);
    vl->addWidget(devBox);

    QDialogButtonBox* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    vl->addWidget(bb);
    connect(bb, &QDialogButtonBox::accepted, &dlg, [&]{
        if (nameEdit->text().isEmpty()) {
            QMessageBox::warning(&dlg, "错误", "场景名称不能为空"); return;
        }
        if (DatabaseManager::instance()->addScene(nameEdit->text(), descEdit->text())) {
            // 获取新场景ID
            auto scenes = DatabaseManager::instance()->getScenes();
            int newId = -1;
            for (const auto& s : scenes)
                if (s["name"].toString() == nameEdit->text()) { newId = s["id"].toInt(); break; }
            if (newId > 0) {
                for (const auto& pair : devControls) {
                    if (pair.first->isChecked()) {
                        int devId = pair.first->property("deviceId").toInt();
                        DatabaseManager::instance()->addSceneDevice(
                            newId, devId, pair.second->currentText(), "");
                    }
                }
            }
            dlg.accept();
        } else {
            QMessageBox::warning(&dlg, "错误", "场景名称已存在");
        }
    });
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    if (dlg.exec() == QDialog::Accepted) loadScenes();
}

void SceneWidget::onEditScene() {
    if (m_selectedSceneId < 0) {
        QMessageBox::information(this, "提示", "请先选择一个场景");
        return;
    }

    // 获取当前场景的基本信息
    auto scenes = DatabaseManager::instance()->getScenes();
    QString curName, curDesc;
    for (const auto& s : scenes) {
        if (s["id"].toInt() == m_selectedSceneId) {
            curName = s["name"].toString();
            curDesc = s["description"].toString();
            break;
        }
    }

    // 获取当前场景已绑定的设备（用于预置选中状态）
    auto currentDevices = DatabaseManager::instance()->getSceneDevices(m_selectedSceneId);
    QMap<int, QString> deviceActionMap;
    for (const auto& d : currentDevices) {
        deviceActionMap[d["device_id"].toInt()] = d["action"].toString();
    }

    QDialog dlg(this);
    dlg.setWindowTitle("编辑场景");
    dlg.setFixedWidth(500);  // 调宽以容纳设备列表

    QVBoxLayout* mainLayout = new QVBoxLayout(&dlg);

    // 名称和描述输入
    QFormLayout* form = new QFormLayout();
    QLineEdit* nameEdit = new QLineEdit(curName, &dlg);
    QLineEdit* descEdit = new QLineEdit(curDesc, &dlg);
    form->addRow("场景名称:", nameEdit);
    form->addRow("场景描述:", descEdit);
    mainLayout->addLayout(form);

    // ----- 设备选择区域（复制自 onAddScene）-----
    QGroupBox* devBox = new QGroupBox("选择绑定设备及操作", &dlg);
    QVBoxLayout* devVl = new QVBoxLayout(devBox);
    QScrollArea* scroll = new QScrollArea(&dlg);
    QWidget* scrollContent = new QWidget(scroll);
    QVBoxLayout* scrollVl = new QVBoxLayout(scrollContent);

    auto devices = DatabaseManager::instance()->getDevices();
    QList<QPair<QCheckBox*, QComboBox*>> devControls;
    for (const auto& d : devices) {
        int devId = d["id"].toInt();
        QString devName = d["name"].toString();
        QString devType = d["type"].toString();

        QHBoxLayout* row = new QHBoxLayout();
        QCheckBox* cb = new QCheckBox(devName + " [" + devType + "]", scrollContent);
        cb->setProperty("deviceId", devId);

        QComboBox* actionCombo = new QComboBox(scrollContent);
        actionCombo->addItems({"开启", "关闭", "设置参数"});

        // 如果当前设备已绑定，则勾选并设置动作
        if (deviceActionMap.contains(devId)) {
            cb->setChecked(true);
            QString action = deviceActionMap[devId];
            int index = actionCombo->findText(action);
            if (index >= 0) actionCombo->setCurrentIndex(index);
        }

        row->addWidget(cb);
        row->addWidget(actionCombo);
        scrollVl->addLayout(row);
        devControls.append({cb, actionCombo});
    }

    scrollContent->setLayout(scrollVl);
    scroll->setWidget(scrollContent);
    scroll->setFixedHeight(200);
    devVl->addWidget(scroll);
    mainLayout->addWidget(devBox);
    // --------------------------------------------

    QDialogButtonBox* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    mainLayout->addWidget(bb);

    connect(bb, &QDialogButtonBox::accepted, &dlg, [&] {
        // 1. 更新场景名称和描述
        DatabaseManager::instance()->updateScene(m_selectedSceneId, nameEdit->text(), descEdit->text());

        // 2. 删除旧的设备绑定（使用已有的批量删除方法）
        DatabaseManager::instance()->deleteSceneDevices(m_selectedSceneId);

        // 3. 添加新的设备绑定
        for (const auto& pair : devControls) {
            if (pair.first->isChecked()) {
                int devId = pair.first->property("deviceId").toInt();
                QString action = pair.second->currentText();
                DatabaseManager::instance()->addSceneDevice(m_selectedSceneId, devId, action, "");
            }
        }

        dlg.accept();
    });

    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        loadScenes();  // 刷新列表
        // 重新选中当前编辑的场景（因为列表刷新后选中状态会丢失）
        for (int i = 0; i < m_sceneList->count(); ++i) {
            QListWidgetItem* item = m_sceneList->item(i);
            if (item->data(Qt::UserRole).toInt() == m_selectedSceneId) {
                m_sceneList->setCurrentRow(i);
                break;
            }
        }
    }
}

void SceneWidget::onDeleteScene() {
    if (m_selectedSceneId < 0) { QMessageBox::information(this,"提示","请先选择一个场景"); return; }
    if (QMessageBox::question(this,"确认","确定要删除该场景吗？") == QMessageBox::Yes) {
        DatabaseManager::instance()->deleteScene(m_selectedSceneId);
        m_selectedSceneId = -1;
        m_activateBtn->setEnabled(false);
        m_descLbl->setText("请选择一个场景");
        m_deviceList->clear();
        loadScenes();
    }
}

void SceneWidget::onActivateScene() {
    if (m_selectedSceneId < 0) return;
    auto devs = DatabaseManager::instance()->getSceneDevices(m_selectedSceneId);
    int count = 0;
    for (const auto& d : devs) {
        QString action = d["action"].toString();
        QString newStatus = (action == "关闭") ? "offline" : "online";
        DatabaseManager::instance()->updateDeviceStatus(d["device_id"].toInt(), newStatus, d["params"].toString());
        DatabaseManager::instance()->addOperationLog(m_username, d["name"].toString(),
                                                      "场景触发:" + action, "success");
        count++;
    }
    QMessageBox::information(this, "场景激活", QString("场景已激活，共执行 %1 个设备操作").arg(count));
}
