#include "scenewidget.h"
#include "databasemanager.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMap>
#include <QMessageBox>
#include <QPair>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QVariant>
#include <QVBoxLayout>

// 场景页：维护场景定义、设备绑定和场景激活执行。

namespace {
using ActionOption = QPair<QString, QString>; // action label, params template

QMap<QString, QString> parseParams(const QString& params) {
    QMap<QString, QString> kv;
    const QStringList parts = params.split(';', Qt::SkipEmptyParts);
    for (const QString& p : parts) {
        const int i = p.indexOf('=');
        if (i <= 0) {
            continue;
        }
        kv[p.left(i).trimmed()] = p.mid(i + 1).trimmed();
    }
    return kv;
}

QString buildParams(const QMap<QString, QString>& kv) {
    QStringList out;
    for (auto it = kv.constBegin(); it != kv.constEnd(); ++it) {
        out << QString("%1=%2").arg(it.key(), it.value());
    }
    return out.join(';');
}

QString mergeParams(const QString& base, const QString& updates) {
    QMap<QString, QString> kv = parseParams(base);
    const QMap<QString, QString> patch = parseParams(updates);
    for (auto it = patch.constBegin(); it != patch.constEnd(); ++it) {
        kv[it.key()] = it.value();
    }
    return buildParams(kv);
}

QString buttonStyle(const QString& color,
                    const QString& hoverColor,
                    const QString& pressedColor,
                    const QString& padding = "5px 12px",
                    int radius = 4,
                    const QString& extra = QString()) {
    return QString("QPushButton{background:%1;color:white;padding:%2;border-radius:%3px;%4}"
                   "QPushButton:hover{background:%5;}"
                   "QPushButton:pressed{background:%6;}"
                   "QPushButton:disabled{background:#cbd5e1;color:#f8fafc;}")
        .arg(color, padding, QString::number(radius), extra, hoverColor, pressedColor);
}

void styleDialogButtons(QDialogButtonBox* buttonBox) {
    if (!buttonBox) {
        return;
    }

    if (QPushButton* okBtn = buttonBox->button(QDialogButtonBox::Ok)) {
        okBtn->setStyleSheet(buttonStyle("#3498db", "#5dade2", "#1f6a9a"));
    }
    if (QPushButton* cancelBtn = buttonBox->button(QDialogButtonBox::Cancel)) {
        cancelBtn->setStyleSheet(buttonStyle("#7f8c8d", "#95a5a6", "#5d6d6f"));
    }
}

QList<ActionOption> actionOptionsForDevice(const QString& type, const QString& name) {
    // 根据设备类型生成可选动作模板，减少手工输入参数。
    QList<ActionOption> options;
    if (type == "空调") {
        options << ActionOption("开机-制冷24中风", "power=on;mode=制冷;temp=24;fan=中")
                << ActionOption("开机-制热26低风", "power=on;mode=制热;temp=26;fan=低")
                << ActionOption("开机-除湿", "power=on;mode=除湿;fan=自动")
                << ActionOption("关机", "power=off");
    } else if (type == "灯光") {
        const bool supportsColor = name.contains("客厅");
        options << ActionOption("开灯", "power=on");
        if (supportsColor) {
            options << ActionOption("开灯-暖白80%", "power=on;brightness=80;color=暖白")
                    << ActionOption("开灯-夜灯30%", "power=on;brightness=30;color=暖白");
        }
        options << ActionOption("关灯", "power=off");
    } else if (type == "窗帘") {
        options << ActionOption("打开窗帘", "power=on;open=100")
                << ActionOption("半开窗帘", "power=on;open=50")
                << ActionOption("关闭窗帘", "power=off;open=0");
    } else if (type == "摄像头") {
        options << ActionOption("开启监控", "power=on")
                << ActionOption("关闭监控", "power=off");
    } else if (type == "空气净化器") {
        options << ActionOption("开启净化", "power=on")
                << ActionOption("关闭净化", "power=off");
    } else {
        options << ActionOption("开机", "power=on")
                << ActionOption("关机", "power=off");
    }
    return options;
}
} // namespace

SceneWidget::SceneWidget(const QString& username, QWidget* parent)
    : QWidget(parent), m_username(username) {
    setupUI();
    loadScenes();
}

void SceneWidget::setupUI() {
    QVBoxLayout* vl = new QVBoxLayout(this);
    vl->setContentsMargins(12, 12, 12, 12);

    QLabel* title = new QLabel("场景模式设置", this);
    title->setStyleSheet("font-size:16px;font-weight:bold;color:#2c3e50;");
    vl->addWidget(title);

    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);

    QWidget* left = new QWidget(splitter);
    QVBoxLayout* leftVl = new QVBoxLayout(left);
    leftVl->setContentsMargins(0, 0, 0, 0);
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
        QString hoverColor = color;
        QString pressedColor = color;
        if (color == "#27ae60") {
            hoverColor = "#52be80";
            pressedColor = "#1e8449";
        } else if (color == "#e67e22") {
            hoverColor = "#eb984e";
            pressedColor = "#af601a";
        } else if (color == "#e74c3c") {
            hoverColor = "#ec7063";
            pressedColor = "#b03a2e";
        }
        btn->setStyleSheet(buttonStyle(color, hoverColor, pressedColor, "5px 12px", 4));
        connect(btn, SIGNAL(clicked()), this, slot);
        btnRow->addWidget(btn);
    };
    makeBtn("➕ 新建", SLOT(onAddScene()), "#27ae60");
    makeBtn("✏️ 编辑", SLOT(onEditScene()), "#e67e22");
    makeBtn("🗑 删除", SLOT(onDeleteScene()), "#e74c3c");
    leftVl->addLayout(btnRow);

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
    m_deviceList->setMaximumHeight(220);
    devVl->addWidget(m_deviceList);
    rightVl->addWidget(devBox);

    m_activateBtn = new QPushButton("▶ 激活此场景", right);
    m_activateBtn->setFixedHeight(44);
    m_activateBtn->setEnabled(false);
    m_activateBtn->setStyleSheet(buttonStyle("#2c3e50", "#34495e", "#1f2d3a", "8px 16px", 6, "font-size:14px;"));
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
    if (row < 0) {
        return;
    }
    QListWidgetItem* item = m_sceneList->item(row);
    if (!item) {
        return;
    }
    m_selectedSceneId = item->data(Qt::UserRole).toInt();

    auto scenes = DatabaseManager::instance()->getScenes();
    for (const auto& s : scenes) {
        if (s["id"].toInt() == m_selectedSceneId) {
            m_descLbl->setText(s["description"].toString());
            break;
        }
    }

    m_deviceList->clear();
    auto devs = DatabaseManager::instance()->getSceneDevices(m_selectedSceneId);
    for (const auto& d : devs) {
        const QString params = d["params"].toString();
        m_deviceList->addItem(QString("📌 %1 → %2%3")
                                  .arg(d["name"].toString(),
                                       d["action"].toString(),
                                       params.isEmpty() ? "" : (" (" + params + ")")));
    }
    m_activateBtn->setEnabled(true);
}

void SceneWidget::onAddScene() {
    QDialog dlg(this);
    dlg.setWindowTitle("新建场景");
    dlg.setFixedWidth(520);
    QVBoxLayout* vl = new QVBoxLayout(&dlg);

    QFormLayout* form = new QFormLayout();
    QLineEdit* nameEdit = new QLineEdit(&dlg);
    QLineEdit* descEdit = new QLineEdit(&dlg);
    form->addRow("场景名称:", nameEdit);
    form->addRow("场景描述:", descEdit);
    vl->addLayout(form);

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
        const auto options = actionOptionsForDevice(d["type"].toString(), d["name"].toString());
        for (const auto& opt : options) {
            actionCombo->addItem(opt.first, opt.second);
        }

        row->addWidget(cb);
        row->addWidget(actionCombo);
        scrollVl->addLayout(row);
        devControls.append(qMakePair(cb, actionCombo));
    }
    scrollContent->setLayout(scrollVl);
    scroll->setWidget(scrollContent);
    scroll->setFixedHeight(220);
    devVl->addWidget(scroll);
    vl->addWidget(devBox);

    QDialogButtonBox* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    styleDialogButtons(bb);
    vl->addWidget(bb);
    connect(bb, &QDialogButtonBox::accepted, &dlg, [&] {
        if (nameEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(&dlg, "错误", "场景名称不能为空");
            return;
        }
        if (!DatabaseManager::instance()->addScene(nameEdit->text().trimmed(), descEdit->text().trimmed())) {
            QMessageBox::warning(&dlg, "错误", "场景名称已存在");
            return;
        }

        auto scenes = DatabaseManager::instance()->getScenes();
        int newId = -1;
        for (const auto& s : scenes) {
            if (s["name"].toString() == nameEdit->text().trimmed()) {
                newId = s["id"].toInt();
                break;
            }
        }

        if (newId > 0) {
            for (const auto& pair : devControls) {
                if (!pair.first->isChecked()) {
                    continue;
                }
                const int devId = pair.first->property("deviceId").toInt();
                const QString action = pair.second->currentText();
                const QString params = pair.second->currentData().toString();
                DatabaseManager::instance()->addSceneDevice(newId, devId, action, params);
            }
        }
        dlg.accept();
    });
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        loadScenes();
    }
}

void SceneWidget::onEditScene() {
    if (m_selectedSceneId < 0) {
        QMessageBox::information(this, "提示", "请先选择一个场景");
        return;
    }

    auto scenes = DatabaseManager::instance()->getScenes();
    QString curName;
    QString curDesc;
    for (const auto& s : scenes) {
        if (s["id"].toInt() == m_selectedSceneId) {
            curName = s["name"].toString();
            curDesc = s["description"].toString();
            break;
        }
    }

    auto currentDevices = DatabaseManager::instance()->getSceneDevices(m_selectedSceneId);
    QMap<int, QPair<QString, QString>> selectedMap; // deviceId -> (action, params)
    for (const auto& d : currentDevices) {
        selectedMap[d["device_id"].toInt()] = qMakePair(d["action"].toString(), d["params"].toString());
    }

    QDialog dlg(this);
    dlg.setWindowTitle("编辑场景");
    dlg.setFixedWidth(520);
    QVBoxLayout* mainLayout = new QVBoxLayout(&dlg);

    QFormLayout* form = new QFormLayout();
    QLineEdit* nameEdit = new QLineEdit(curName, &dlg);
    QLineEdit* descEdit = new QLineEdit(curDesc, &dlg);
    form->addRow("场景名称:", nameEdit);
    form->addRow("场景描述:", descEdit);
    mainLayout->addLayout(form);

    QGroupBox* devBox = new QGroupBox("选择绑定设备及操作", &dlg);
    QVBoxLayout* devVl = new QVBoxLayout(devBox);
    QScrollArea* scroll = new QScrollArea(&dlg);
    QWidget* scrollContent = new QWidget(scroll);
    QVBoxLayout* scrollVl = new QVBoxLayout(scrollContent);

    auto devices = DatabaseManager::instance()->getDevices();
    QList<QPair<QCheckBox*, QComboBox*>> devControls;
    for (const auto& d : devices) {
        const int devId = d["id"].toInt();
        QHBoxLayout* row = new QHBoxLayout();

        QCheckBox* cb = new QCheckBox(d["name"].toString() + " [" + d["type"].toString() + "]", scrollContent);
        cb->setProperty("deviceId", devId);

        QComboBox* actionCombo = new QComboBox(scrollContent);
        const auto options = actionOptionsForDevice(d["type"].toString(), d["name"].toString());
        for (const auto& opt : options) {
            actionCombo->addItem(opt.first, opt.second);
        }

        if (selectedMap.contains(devId)) {
            cb->setChecked(true);
            const QString action = selectedMap[devId].first;
            const QString params = selectedMap[devId].second;
            int idx = actionCombo->findText(action);
            if (idx < 0) {
                idx = actionCombo->findData(params);
            }
            if (idx >= 0) {
                actionCombo->setCurrentIndex(idx);
            }
        }

        row->addWidget(cb);
        row->addWidget(actionCombo);
        scrollVl->addLayout(row);
        devControls.append(qMakePair(cb, actionCombo));
    }

    scrollContent->setLayout(scrollVl);
    scroll->setWidget(scrollContent);
    scroll->setFixedHeight(220);
    devVl->addWidget(scroll);
    mainLayout->addWidget(devBox);

    QDialogButtonBox* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    styleDialogButtons(bb);
    mainLayout->addWidget(bb);

    connect(bb, &QDialogButtonBox::accepted, &dlg, [&] {
        DatabaseManager::instance()->updateScene(m_selectedSceneId, nameEdit->text().trimmed(), descEdit->text().trimmed());
        DatabaseManager::instance()->deleteSceneDevices(m_selectedSceneId);

        for (const auto& pair : devControls) {
            if (!pair.first->isChecked()) {
                continue;
            }
            const int devId = pair.first->property("deviceId").toInt();
            const QString action = pair.second->currentText();
            const QString params = pair.second->currentData().toString();
            DatabaseManager::instance()->addSceneDevice(m_selectedSceneId, devId, action, params);
        }
        dlg.accept();
    });

    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        loadScenes();
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
    if (m_selectedSceneId < 0) {
        QMessageBox::information(this, "提示", "请先选择一个场景");
        return;
    }
    if (QMessageBox::question(this, "确认", "确定要删除该场景吗？") != QMessageBox::Yes) {
        return;
    }

    DatabaseManager::instance()->deleteScene(m_selectedSceneId);
    m_selectedSceneId = -1;
    m_activateBtn->setEnabled(false);
    m_descLbl->setText("请选择一个场景");
    m_deviceList->clear();
    loadScenes();
}

void SceneWidget::onActivateScene() {
    // 执行场景时跳过离线设备，并记录执行/跳过日志。
    if (m_selectedSceneId < 0) {
        return;
    }

    auto devs = DatabaseManager::instance()->getSceneDevices(m_selectedSceneId);
    int executed = 0;
    int skippedOffline = 0;

    for (const auto& d : devs) {
        const int devId = d["device_id"].toInt();
        const auto device = DatabaseManager::instance()->getDeviceById(devId);
        if (device.isEmpty()) {
            continue;
        }

        if (device["status"].toString() != "online") {
            DatabaseManager::instance()->addOperationLog(
                m_username,
                d["name"].toString(),
                "场景跳过:" + d["action"].toString(),
                "offline_skip");
            ++skippedOffline;
            continue;
        }

        const QString merged = d["params"].toString().isEmpty()
                                   ? device["params"].toString()
                                   : mergeParams(device["params"].toString(), d["params"].toString());

        DatabaseManager::instance()->updateDeviceStatus(devId, "online", merged);
        DatabaseManager::instance()->addOperationLog(
            m_username,
            d["name"].toString(),
            "场景触发:" + d["action"].toString(),
            "success");
        ++executed;
    }

    QMessageBox::information(this,
                             "场景激活",
                             QString("场景已激活，执行 %1 个操作，跳过离线设备 %2 个。")
                                 .arg(executed)
                                 .arg(skippedOffline));
}
