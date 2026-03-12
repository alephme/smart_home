#include "homewidget.h"
#include "databasemanager.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QGroupBox>
#include <QFrame>
#include <QMessageBox>
#include <QTimer>
#include <QDateTime>
#include <QMap>

namespace {
const char* kFavoriteSceneSettingKey = "home_favorite_scene_ids";
const QSize kSceneCardSize(132, 52);
const char* kSceneCardColor = "#1f4b99";

QString joinIds(const QList<int>& ids) {
    QStringList out;
    for (const int id : ids) {
        out << QString::number(id);
    }
    return out.join(',');
}

QList<int> parseIds(const QString& text) {
    QList<int> ids;
    const QStringList parts = text.split(',', Qt::SkipEmptyParts);
    for (const QString& p : parts) {
        bool ok = false;
        const int id = p.toInt(&ok);
        if (ok && !ids.contains(id)) {
            ids.append(id);
        }
    }
    return ids;
}

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

QString buildParams(const QMap<QString, QString>& kv) {
    QStringList parts;
    for (auto it = kv.constBegin(); it != kv.constEnd(); ++it) {
        parts << QString("%1=%2").arg(it.key(), it.value());
    }
    return parts.join(';');
}

QString mergeParams(const QString& base, const QString& updates) {
    QMap<QString, QString> kv = parseParams(base);
    const QMap<QString, QString> patch = parseParams(updates);
    for (auto it = patch.constBegin(); it != patch.constEnd(); ++it) {
        kv[it.key()] = it.value();
    }
    return buildParams(kv);
}
}

static QWidget* makeCard(const QString& title, const QString& iconText,
                          const QString& color, QLabel*& valueLbl, QWidget* parent) {
    QFrame* card = new QFrame(parent);
    card->setFixedSize(180, 110);
    card->setStyleSheet(QString("QFrame{background:%1;border-radius:10px;}").arg(color));

    QVBoxLayout* vl = new QVBoxLayout(card);
    QLabel* icon = new QLabel(iconText, card);
    icon->setAlignment(Qt::AlignHCenter);
    icon->setStyleSheet("font-size:28px;");

    valueLbl = new QLabel("0", card);
    valueLbl->setAlignment(Qt::AlignHCenter);
    valueLbl->setStyleSheet("color:white;font-size:28px;font-weight:bold;");

    QLabel* titleLbl = new QLabel(title, card);
    titleLbl->setAlignment(Qt::AlignHCenter);
    titleLbl->setStyleSheet("color:rgba(255,255,255,0.85);font-size:13px;");

    vl->addWidget(icon);
    vl->addWidget(valueLbl);
    vl->addWidget(titleLbl);
    return card;
}

HomeWidget::HomeWidget(const QString& username, QWidget* parent)
    : QWidget(parent), m_username(username) {
    setupUI();
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &HomeWidget::refresh);
    m_timer->start(5000);
    refresh();
}

void HomeWidget::setupUI() {
    QVBoxLayout* vl = new QVBoxLayout(this);
    vl->setContentsMargins(20,20,20,20);
    vl->setSpacing(16);

    // 欢迎语
    QLabel* welcome = new QLabel(QString("欢迎回来，%1 ！").arg(m_username), this);
    welcome->setStyleSheet("font-size:20px;font-weight:bold;color:#2c3e50;");
    m_timeLbl = new QLabel(this);
    m_timeLbl->setStyleSheet("color:#7f8c8d;font-size:13px;");
    vl->addWidget(welcome);
    vl->addWidget(m_timeLbl);

    // 统计卡片
    QGroupBox* statsBox = new QGroupBox("设备概览", this);
    QHBoxLayout* cardsLay = new QHBoxLayout(statsBox);
    cardsLay->setSpacing(16);
    cardsLay->addWidget(makeCard("在线设备", "✅", "#27ae60", m_onlineLbl, statsBox));
    cardsLay->addWidget(makeCard("离线设备", "❌", "#e74c3c", m_offlineLbl, statsBox));
    cardsLay->addWidget(makeCard("当前报警", "🔔", "#e67e22", m_alarmLbl, statsBox));
    cardsLay->addStretch();
    vl->addWidget(statsBox);

    // 快捷控制
    QGroupBox* quickBox = new QGroupBox("快捷操作", this);
    QGridLayout* grid = new QGridLayout(quickBox);
    grid->setSpacing(10);

    auto makeQuick = [&](const QString& label, int page, int row, int col) {
        QPushButton* btn = new QPushButton(label, quickBox);
        btn->setFixedHeight(60);
        btn->setStyleSheet("QPushButton{background:#3498db;color:white;border-radius:8px;font-size:14px;}"
                           "QPushButton:hover{background:#2980b9;}");
        connect(btn, &QPushButton::clicked, [this, page]{ emit navigateTo(page); });
        grid->addWidget(btn, row, col);
    };
    makeQuick("📱 设备控制", 1, 0, 0);
    makeQuick("🎬 场景模式", 2, 0, 1);
    makeQuick("📊 历史记录", 3, 0, 2);
    makeQuick("🔔 报警管理", 4, 1, 0);
    makeQuick("⚙️  系统设置", 5, 1, 1);
    vl->addWidget(quickBox);

    // 场景快捷切换
    QGroupBox* sceneBox = new QGroupBox("常用场景", this);
    m_sceneLay = new QHBoxLayout(sceneBox);
    m_sceneLay->setSpacing(10);
    vl->addWidget(sceneBox);
    vl->addStretch();

    refreshFavoriteScenes();
}

void HomeWidget::refresh() {
    m_timeLbl->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    updateStats();
    refreshFavoriteScenes();
}

void HomeWidget::updateStats() {
    auto devices = DatabaseManager::instance()->getDevices();
    int online = 0, offline = 0;
    for (const auto& d : devices) {
        if (d["status"].toString() == "online") online++;
        else offline++;
    }
    auto alarms = DatabaseManager::instance()->getAlarms();
    m_onlineLbl->setText(QString::number(online));
    m_offlineLbl->setText(QString::number(offline));
    m_alarmLbl->setText(QString::number(alarms.size()));
}

QList<int> HomeWidget::loadFavoriteSceneIds() const {
    return parseIds(DatabaseManager::instance()->getSetting(kFavoriteSceneSettingKey));
}

void HomeWidget::saveFavoriteSceneIds(const QList<int>& ids) const {
    DatabaseManager::instance()->setSetting(kFavoriteSceneSettingKey, joinIds(ids));
}

void HomeWidget::refreshFavoriteScenes() {
    if (!m_sceneLay) {
        return;
    }

    while (QLayoutItem* item = m_sceneLay->takeAt(0)) {
        if (QWidget* w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }

    const auto scenes = DatabaseManager::instance()->getScenes();
    QMap<int, QString> sceneIdName;
    for (const auto& s : scenes) {
        sceneIdName[s.value("id").toInt()] = s.value("name").toString();
    }

    QList<int> favoriteIds = loadFavoriteSceneIds();
    QList<int> validIds;
    for (const int id : favoriteIds) {
        if (sceneIdName.contains(id)) {
            validIds.append(id);
        }
    }

    if (validIds != favoriteIds) {
        saveFavoriteSceneIds(validIds);
    }

    if (validIds.isEmpty() && !scenes.isEmpty()) {
        for (int i = 0; i < scenes.size() && i < 4; ++i) {
            validIds.append(scenes.at(i).value("id").toInt());
        }
        saveFavoriteSceneIds(validIds);
    }

    for (const int id : validIds) {
        const QString sceneName = sceneIdName.value(id);
        if (sceneName.isEmpty()) {
            continue;
        }

        QPushButton* btn = new QPushButton("🎬 " + sceneName, this);
        btn->setFixedSize(kSceneCardSize);
        btn->setStyleSheet(QString("QPushButton{background:%1;color:white;border-radius:6px;font-size:13px;padding:0 10px;}"
                                   "QPushButton:hover{background:#2a5fbf;}"
                                   "QPushButton:pressed{background:#173a79;}")
                               .arg(kSceneCardColor));
        connect(btn, &QPushButton::clicked, [this, id] { activateFavoriteScene(id); });
        m_sceneLay->addWidget(btn);
    }

    QPushButton* addCard = new QPushButton("＋", this);
    addCard->setToolTip("编辑常用场景（添加或删除）");
    addCard->setFixedSize(kSceneCardSize);
    addCard->setStyleSheet(QString(
        "QPushButton{background:%1;color:white;border-radius:6px;font-size:24px;font-weight:bold;}"
        "QPushButton:hover{background:#2a5fbf;}"
        "QPushButton:pressed{background:#173a79;}")
        .arg(kSceneCardColor));
    connect(addCard, &QPushButton::clicked, this, &HomeWidget::openFavoriteSceneEditor);
    m_sceneLay->addWidget(addCard);
    m_sceneLay->addStretch();
}

void HomeWidget::activateFavoriteScene(int sceneId) {
    const auto sceneDevices = DatabaseManager::instance()->getSceneDevices(sceneId);
    if (sceneDevices.isEmpty()) {
        QMessageBox::information(this, "提示", "该场景未绑定任何设备动作。");
        return;
    }

    int executed = 0;
    int skippedOffline = 0;
    for (const auto& d : sceneDevices) {
        const int deviceId = d.value("device_id").toInt();
        const auto device = DatabaseManager::instance()->getDeviceById(deviceId);
        if (device.isEmpty()) {
            continue;
        }

        if (device.value("status").toString() != "online") {
            DatabaseManager::instance()->addOperationLog(
                m_username,
                d.value("name").toString(),
                "场景跳过:" + d.value("action").toString(),
                "offline_skip");
            ++skippedOffline;
            continue;
        }

        const QString merged = d.value("params").toString().isEmpty()
                                   ? device.value("params").toString()
                                   : mergeParams(device.value("params").toString(), d.value("params").toString());

        DatabaseManager::instance()->updateDeviceStatus(deviceId, "online", merged);
        DatabaseManager::instance()->addOperationLog(
            m_username,
            d.value("name").toString(),
            "首页场景触发:" + d.value("action").toString(),
            "success");
        ++executed;
    }

    QMessageBox::information(this,
                             "场景激活",
                             QString("场景已激活，执行 %1 个操作，跳过离线设备 %2 个。")
                                 .arg(executed)
                                 .arg(skippedOffline));
    refresh();
}

void HomeWidget::openFavoriteSceneEditor() {
    const auto scenes = DatabaseManager::instance()->getScenes();
    if (scenes.isEmpty()) {
        QMessageBox::information(this, "提示", "当前没有可选场景，请先在场景模式中创建场景。");
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle("编辑常用场景");
    dlg.setMinimumWidth(360);

    QVBoxLayout* vl = new QVBoxLayout(&dlg);
    QLabel* tip = new QLabel("勾选要在首页展示的常用场景（仅可选择当前已有场景）", &dlg);
    tip->setWordWrap(true);
    tip->setStyleSheet("color:#475569;");
    vl->addWidget(tip);

    QList<int> selectedIds = loadFavoriteSceneIds();
    QList<QCheckBox*> boxes;
    for (const auto& s : scenes) {
        const int id = s.value("id").toInt();
        QCheckBox* cb = new QCheckBox(s.value("name").toString(), &dlg);
        cb->setChecked(selectedIds.contains(id));
        cb->setProperty("sceneId", id);
        vl->addWidget(cb);
        boxes.append(cb);
    }

    QDialogButtonBox* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    vl->addWidget(bb);
    connect(bb, &QDialogButtonBox::accepted, &dlg, [&] {
        QList<int> nextIds;
        for (QCheckBox* cb : boxes) {
            if (!cb->isChecked()) {
                continue;
            }
            nextIds.append(cb->property("sceneId").toInt());
        }
        saveFavoriteSceneIds(nextIds);
        dlg.accept();
    });
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        refreshFavoriteScenes();
    }
}
