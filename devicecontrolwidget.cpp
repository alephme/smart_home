#include "devicecontrolwidget.h"
#include "databasemanager.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QApplication>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QStyle>
#include <QStringList>
#include <QSpinBox>
#include <QTcpSocket>
#include <QVBoxLayout>

#include <algorithm>

namespace {
QIcon iconForDeviceType(const QString& type, QWidget* host) {
    QStyle* s = host ? host->style() : QApplication::style();
    if (type == "空调") {
        const QIcon icon(":/icons/device_air.png");
        if (!icon.isNull()) {
            return icon;
        }
        return s->standardIcon(QStyle::SP_BrowserReload);
    }
    if (type == "灯光") {
        const QIcon icon(":/icons/device_light.png");
        if (!icon.isNull()) {
            return icon;
        }
        return s->standardIcon(QStyle::SP_DialogYesButton);
    }
    if (type == "窗帘") {
        const QIcon icon(":/icons/device_curtain.png");
        if (!icon.isNull()) {
            return icon;
        }
        return s->standardIcon(QStyle::SP_ArrowLeft);
    }
    if (type == "摄像头") {
        const QIcon icon(":/icons/device_camera.png");
        if (!icon.isNull()) {
            return icon;
        }
        return s->standardIcon(QStyle::SP_ComputerIcon);
    }
    return s->standardIcon(QStyle::SP_FileIcon);
}
}

DeviceControlWidget::DeviceControlWidget(const QString& username, QWidget* parent)
    : QWidget(parent), m_username(username) {
    setupUI();
    loadDevices();
}

QMap<QString, QString> DeviceControlWidget::parseParams(const QString& params) {
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

QString DeviceControlWidget::buildParams(const QMap<QString, QString>& kv) {
    QStringList parts;
    for (auto it = kv.constBegin(); it != kv.constEnd(); ++it) {
        parts << QString("%1=%2").arg(it.key(), it.value());
    }
    return parts.join(';');
}

void DeviceControlWidget::setupUI() {
    QVBoxLayout* outerVl = new QVBoxLayout(this);
    outerVl->setContentsMargins(12, 12, 12, 12);

    QHBoxLayout* topBar = new QHBoxLayout();
    QLabel* titleLbl = new QLabel("设备控制与管理", this);
    titleLbl->setStyleSheet("font-size:16px;font-weight:bold;color:#2c3e50;");
    topBar->addWidget(titleLbl);
    topBar->addStretch();

    QLabel* filterLbl = new QLabel("类型过滤:", this);
    m_filterCombo = new QComboBox(this);
    m_filterCombo->addItems({"全部", "灯光", "空调", "窗帘", "摄像头"});
    connect(m_filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DeviceControlWidget::onRefresh);

    auto makeBtn = [this, topBar](const QString& text, const QString& color, auto slot) {
        QPushButton* btn = new QPushButton(text, this);
        btn->setStyleSheet(QString("QPushButton{background:%1;color:white;padding:5px 12px;border-radius:4px;}"
                                   "QPushButton:hover{opacity:0.85;}")
                               .arg(color));
        connect(btn, &QPushButton::clicked, this, slot);
        topBar->addWidget(btn);
    };

    topBar->addWidget(filterLbl);
    topBar->addWidget(m_filterCombo);
    makeBtn("刷新", "#3498db", &DeviceControlWidget::onRefresh);
    makeBtn("添加设备", "#27ae60", &DeviceControlWidget::onAddDevice);
    outerVl->addLayout(topBar);

    m_cardList = new QListWidget(this);
    m_cardList->setViewMode(QListView::IconMode);
    m_cardList->setFlow(QListView::LeftToRight);
    m_cardList->setWrapping(true);
    m_cardList->setResizeMode(QListView::Adjust);
    m_cardList->setMovement(QListView::Snap);
    m_cardList->setDragDropMode(QAbstractItemView::NoDragDrop);
    m_cardList->setDragDropOverwriteMode(false);
    m_cardList->setDefaultDropAction(Qt::MoveAction);
    m_cardList->setDragEnabled(false);
    m_cardList->setAcceptDrops(false);
    m_cardList->setDropIndicatorShown(false);
    m_cardList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_cardList->setSpacing(12);
    m_cardList->setFrameShape(QFrame::NoFrame);
    m_cardList->setGridSize(QSize(280, 138));
    m_cardList->setIconSize(QSize(36, 36));
    m_cardList->setContextMenuPolicy(Qt::CustomContextMenu);
    m_cardList->setStyleSheet(
        "QListWidget{background:#e6edf5;border:none;}"
        "QListWidget::item{background:#ffffff;border:1px solid #93a6bb;border-radius:10px;padding:10px;text-align:left;color:#1f2937;}"
        "QListWidget::item:hover{border:1px solid #3b82f6;background:#f8fbff;}"
        "QListWidget::item:selected{border:2px solid #16a34a;color:#1f2937;background:#ffffff;}"
        "QListWidget::item:selected:active{background:#ffffff;color:#2c3e50;}"
    );
    m_cardList->viewport()->installEventFilter(this);

    connect(m_cardList, &QListWidget::itemClicked, this, &DeviceControlWidget::onCardItemClicked);
    connect(m_cardList, &QWidget::customContextMenuRequested, this, &DeviceControlWidget::onCardContextMenu);
    outerVl->addWidget(m_cardList);
}

void DeviceControlWidget::addDeviceCard(const QMap<QString, QVariant>& device) {
    const int id = device.value("id").toInt();
    const bool isOnline = device.value("status").toString() == "online";
    const QString name = device.value("name").toString();
    const QString type = device.value("type").toString();
    const QString group = device.value("grp").toString();
    const QString status = isOnline ? "在线" : "离线";
    const QString tooltip = QString("设备: %1\n类型: %2\n分组: %3\n状态: %4")
                                .arg(name, type, group, status);

    QListWidgetItem* item = new QListWidgetItem(iconForDeviceType(type, m_cardList), name, m_cardList);
    item->setToolTip(tooltip);
    item->setData(Qt::UserRole, id);
    item->setSizeHint(QSize(280, 138));
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    item->setFlags(item->flags() | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsSelectable);
    if (id == m_selectedDeviceId) {
        item->setSelected(true);
    }

    if (isOnline) {
        item->setBackground(QBrush(QColor("#ffffff")));
        item->setForeground(QBrush(QColor("#1f2937")));
    } else {
        item->setBackground(QBrush(QColor("#d7dde5")));
        item->setForeground(QBrush(QColor("#4b5563")));
    }
}

void DeviceControlWidget::loadDevices() {
    m_cardList->clear();

    m_devices = DatabaseManager::instance()->getDevices();
    restoreDeviceOrderFromSetting();
    std::sort(m_devices.begin(), m_devices.end(), [this](const QMap<QString, QVariant>& a,
                                                         const QMap<QString, QVariant>& b) {
        const int idA = a.value("id").toInt();
        const int idB = b.value("id").toInt();
        return orderRank(idA) < orderRank(idB);
    });

    const QString filter = (m_filterCombo->currentIndex() == 0) ? QString() : m_filterCombo->currentText();
    const bool allVisible = filter.isEmpty();
    m_cardList->setDragDropMode(QAbstractItemView::NoDragDrop);
    m_cardList->setDragEnabled(false);
    m_cardList->setAcceptDrops(false);
    m_cardList->setProperty("reorderEnabled", allVisible);

    for (const auto& d : m_devices) {
        if (!filter.isEmpty() && d.value("type").toString() != filter) {
            continue;
        }
        addDeviceCard(d);
    }

    if (m_cardList->count() == 0) {
        QListWidgetItem* empty = new QListWidgetItem("暂无设备，请先添加设备。", m_cardList);
        empty->setFlags(Qt::NoItemFlags);
        empty->setSizeHint(QSize(280, 60));
        empty->setForeground(QBrush(QColor("#6b7280")));
    }
}

void DeviceControlWidget::restoreDeviceOrderFromSetting() {
    m_deviceOrder.clear();
    const QString saved = DatabaseManager::instance()->getSetting("device_card_order");
    if (saved.trimmed().isEmpty()) {
        for (const auto& d : m_devices) {
            m_deviceOrder.append(d.value("id").toInt());
        }
        return;
    }

    const QStringList ids = saved.split(',', Qt::SkipEmptyParts);
    for (const QString& s : ids) {
        bool ok = false;
        const int id = s.toInt(&ok);
        if (ok) {
            m_deviceOrder.append(id);
        }
    }

    for (const auto& d : m_devices) {
        const int id = d.value("id").toInt();
        if (!m_deviceOrder.contains(id)) {
            m_deviceOrder.append(id);
        }
    }
}

void DeviceControlWidget::persistDeviceOrderToSetting() {
    QStringList ids;
    ids.reserve(m_deviceOrder.size());
    for (const int id : m_deviceOrder) {
        ids << QString::number(id);
    }
    DatabaseManager::instance()->setSetting("device_card_order", ids.join(','));
}

int DeviceControlWidget::orderRank(int deviceId) const {
    const int idx = m_deviceOrder.indexOf(deviceId);
    return idx >= 0 ? idx : (1000000 + deviceId);
}

bool DeviceControlWidget::tryPairDevice(int deviceId, const QMap<QString, QVariant>& device) {
    const QString ip = device.value("ip").toString();
    const int port = device.value("port").toInt();

    QTcpSocket sock;
    sock.connectToHost(ip, static_cast<quint16>(port));
    if (!sock.waitForConnected(1500)) {
        QMessageBox::warning(this, "配对失败",
                             QString("设备 %1 配对失败：%2")
                                 .arg(device.value("name").toString(), sock.errorString()));
        DatabaseManager::instance()->updateDeviceStatus(deviceId, "offline", device.value("params").toString());
        return false;
    }

    sock.disconnectFromHost();
    DatabaseManager::instance()->updateDeviceStatus(deviceId, "online", device.value("params").toString());
    QMessageBox::information(this, "配对成功", "设备已上线，可进入控制界面。");
    return true;
}

void DeviceControlWidget::showDeviceControlDialog(const QMap<QString, QVariant>& device) {
    const int id = device.value("id").toInt();
    const QString name = device.value("name").toString();
    const QString type = device.value("type").toString();
    QMap<QString, QString> kv = parseParams(device.value("params").toString());
    if (!kv.contains("power")) {
        kv["power"] = "on";
    }

    QDialog dlg(this);
    dlg.setWindowTitle("设备控制 - " + name);
    dlg.setMinimumWidth(360);
    QVBoxLayout* vl = new QVBoxLayout(&dlg);

    QLabel* basic = new QLabel(QString("设备: %1\n类型: %2\n状态: 在线").arg(name, type), &dlg);
    basic->setStyleSheet("color:#374151;");
    vl->addWidget(basic);

    QGroupBox* box = new QGroupBox("控制项", &dlg);
    QFormLayout* form = new QFormLayout(box);

    QPushButton* powerBtn = new QPushButton(box);
    const bool powerOn = (kv.value("power", "on") != "off");
    powerBtn->setCheckable(true);
    powerBtn->setChecked(powerOn);
    auto refreshPowerBtn = [powerBtn](bool on) {
        powerBtn->setText(on ? "开启" : "关闭");
        powerBtn->setStyleSheet(on
                                    ? "QPushButton{background:#16a34a;color:white;padding:6px 12px;border-radius:6px;}"
                                    : "QPushButton{background:#6b7280;color:white;padding:6px 12px;border-radius:6px;}");
    };
    refreshPowerBtn(powerOn);
    connect(powerBtn, &QPushButton::toggled, &dlg, [refreshPowerBtn](bool on) { refreshPowerBtn(on); });
    form->addRow("开关", powerBtn);

    QSpinBox* tempSpin = nullptr;
    QComboBox* modeCombo = nullptr;
    QComboBox* fanCombo = nullptr;
    QSpinBox* brightSpin = nullptr;
    QComboBox* colorCombo = nullptr;
    QSpinBox* curtainOpen = nullptr;

    if (type == "空调") {
        modeCombo = new QComboBox(box);
        modeCombo->addItems({"制冷", "制热", "除湿", "送风", "自动"});
        modeCombo->setCurrentText(kv.value("mode", "制冷"));
        form->addRow("模式", modeCombo);

        tempSpin = new QSpinBox(box);
        tempSpin->setRange(16, 30);
        tempSpin->setValue(kv.value("temp", "24").toInt());
        form->addRow("温度(℃)", tempSpin);

        fanCombo = new QComboBox(box);
        fanCombo->addItems({"低", "中", "高", "自动"});
        fanCombo->setCurrentText(kv.value("fan", "中"));
        form->addRow("风力", fanCombo);

        const auto env = DatabaseManager::instance()->getEnvData();
        QString envText = "温度: --  湿度: --";
        if (!env.isEmpty()) {
            const auto latest = env.first();
            envText = QString("温度: %1℃  湿度: %2%")
                          .arg(QString::number(latest.value("temperature").toDouble(), 'f', 1),
                               QString::number(latest.value("humidity").toDouble(), 'f', 1));
        }
        QLabel* sensor = new QLabel(envText, box);
        sensor->setStyleSheet("color:#0f766e;font-weight:600;");
        form->addRow("环境传感", sensor);
    } else if (type == "灯光") {
        const bool advanced = name.contains("客厅");
        if (advanced) {
            brightSpin = new QSpinBox(box);
            brightSpin->setRange(0, 100);
            brightSpin->setValue(kv.value("brightness", "80").toInt());
            form->addRow("亮度(%)", brightSpin);

            colorCombo = new QComboBox(box);
            colorCombo->addItems({"暖白", "冷白", "自然光", "蓝色", "红色"});
            colorCombo->setCurrentText(kv.value("color", "暖白"));
            form->addRow("颜色", colorCombo);
        }
    } else if (type == "窗帘") {
        curtainOpen = new QSpinBox(box);
        curtainOpen->setRange(0, 100);
        curtainOpen->setValue(kv.value("open", "50").toInt());
        form->addRow("开合度(%)", curtainOpen);
    } else if (type == "摄像头") {
        // QLabel* readonly = new QLabel("。", box);
        // readonly->setWordWrap(true);
        // readonly->setStyleSheet("color:#6b7280;");
        // form->addRow(readonly);
    }

    vl->addWidget(box);

    QDialogButtonBox* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    vl->addWidget(bb);
    connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    kv["power"] = powerBtn->isChecked() ? "on" : "off";
    if (modeCombo) {
        kv["mode"] = modeCombo->currentText();
    }
    if (tempSpin) {
        kv["temp"] = QString::number(tempSpin->value());
    }
    if (fanCombo) {
        kv["fan"] = fanCombo->currentText();
    }
    if (brightSpin) {
        kv["brightness"] = QString::number(brightSpin->value());
    }
    if (colorCombo) {
        kv["color"] = colorCombo->currentText();
    }
    if (curtainOpen) {
        kv["open"] = QString::number(curtainOpen->value());
    }

    const QString params = buildParams(kv);
    DatabaseManager::instance()->updateDeviceStatus(id, "online", params);
    DatabaseManager::instance()->addOperationLog(m_username, name, "设备控制: " + params, "success");
    emit deviceChanged();
    loadDevices();
}

void DeviceControlWidget::onCardClicked(int deviceId) {
    m_selectedDeviceId = deviceId;
    auto device = DatabaseManager::instance()->getDeviceById(deviceId);
    if (device.isEmpty()) {
        return;
    }

    const bool online = device.value("status").toString() == "online";
    if (!online) {
        if (QMessageBox::question(this, "设备离线",
                                  "该设备当前离线，是否尝试配对上线？") == QMessageBox::Yes) {
            if (tryPairDevice(deviceId, device)) {
                emit deviceChanged();
            }
        }
        loadDevices();
        return;
    }

    showDeviceControlDialog(device);
}

void DeviceControlWidget::onCardItemClicked(QListWidgetItem* item) {
    if (m_suppressCardClick) {
        m_suppressCardClick = false;
        return;
    }
    if (!item) {
        return;
    }
    const int deviceId = item->data(Qt::UserRole).toInt();
    if (deviceId <= 0) {
        return;
    }
    onCardClicked(deviceId);
}

void DeviceControlWidget::onCardContextMenu(const QPoint& pos) {
    QListWidgetItem* item = m_cardList->itemAt(pos);
    if (!item) {
        return;
    }
    const int deviceId = item->data(Qt::UserRole).toInt();
    if (deviceId <= 0) {
        return;
    }

    m_selectedDeviceId = deviceId;
    QMenu menu(this);
    QAction* editAct = menu.addAction("编辑设备");
    QAction* delAct = menu.addAction("删除设备");
    QAction* picked = menu.exec(m_cardList->viewport()->mapToGlobal(pos));
    if (picked == editAct) {
        onEditDevice();
    } else if (picked == delAct) {
        onDeleteDevice();
    }
}

void DeviceControlWidget::moveOrderByIndex(int from, int to) {
    if (from < 0 || from >= m_deviceOrder.size()) {
        return;
    }
    if (to < 0) {
        to = 0;
    }
    if (to >= m_deviceOrder.size()) {
        to = m_deviceOrder.size() - 1;
    }
    if (from == to) {
        return;
    }

    const int moved = m_deviceOrder.at(from);
    m_deviceOrder.removeAt(from);
    m_deviceOrder.insert(to, moved);
}

bool DeviceControlWidget::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_cardList->viewport()) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto* mouseEv = static_cast<QMouseEvent*>(event);
            m_cardDragging = false;
            m_dragRow = -1;
            m_pressPos = mouseEv->pos();

            const QModelIndex idx = m_cardList->indexAt(mouseEv->pos());
            if (!idx.isValid()) {
                m_cardList->clearSelection();
                m_selectedDeviceId = -1;
                return false;
            }

            if (mouseEv->button() == Qt::LeftButton) {
                m_dragRow = idx.row();
            }
        } else if (event->type() == QEvent::MouseMove) {
            auto* mouseEv = static_cast<QMouseEvent*>(event);
            const bool reorderEnabled = m_cardList->property("reorderEnabled").toBool();
            if (!reorderEnabled || !(mouseEv->buttons() & Qt::LeftButton) || m_dragRow < 0) {
                return false;
            }

            if (!m_cardDragging && (mouseEv->pos() - m_pressPos).manhattanLength() < QApplication::startDragDistance()) {
                return false;
            }

            if (!m_cardDragging) {
                m_cardDragging = true;
                QListWidgetItem* draggingItem = m_cardList->item(m_dragRow);
                if (!draggingItem) {
                    m_cardDragging = false;
                    return false;
                }

                const QRect itemRect = m_cardList->visualItemRect(draggingItem);
                const QPixmap snap = m_cardList->viewport()->grab(itemRect);
                if (!m_dragGhost) {
                    m_dragGhost = new QLabel(m_cardList->viewport());
                    m_dragGhost->setAttribute(Qt::WA_TransparentForMouseEvents, true);
                }
                m_dragGhost->setPixmap(snap);
                m_dragGhost->resize(snap.size());
                m_dragHotspot = mouseEv->pos() - itemRect.topLeft();
                m_dragGhost->move(mouseEv->pos() - m_dragHotspot);
                m_dragGhost->show();

                // Hide real card content; ghost follows cursor for smooth dragging.
                draggingItem->setText("");
                draggingItem->setIcon(QIcon());
                draggingItem->setBackground(QBrush(QColor(0, 0, 0, 0)));
                draggingItem->setForeground(QBrush(QColor(0, 0, 0, 0)));
                m_suppressCardClick = true;
            }

            if (m_dragGhost) {
                m_dragGhost->move(mouseEv->pos() - m_dragHotspot);
            }

            QModelIndex targetIdx = m_cardList->indexAt(mouseEv->pos());
            if (!targetIdx.isValid()) {
                return true;
            }
            const int targetRow = targetIdx.row();
            if (targetRow < 0 || targetRow == m_dragRow) {
                return true;
            }

            QListWidgetItem* moving = m_cardList->takeItem(m_dragRow);
            if (!moving) {
                return true;
            }
            m_cardList->insertItem(targetRow, moving);
            m_cardList->setCurrentItem(moving);
            m_selectedDeviceId = moving->data(Qt::UserRole).toInt();

            moveOrderByIndex(m_dragRow, targetRow);
            m_dragRow = targetRow;
            m_suppressCardClick = true;
            return true;
        } else if (event->type() == QEvent::MouseButtonRelease) {
            if (m_cardDragging) {
                persistDeviceOrderToSetting();
                if (m_dragGhost) {
                    m_dragGhost->hide();
                }
                loadDevices();
                m_suppressCardClick = true;
            }
            m_cardDragging = false;
            m_dragRow = -1;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void DeviceControlWidget::onAddDevice() {
    QDialog dlg(this);
    dlg.setWindowTitle("添加设备");
    dlg.setFixedWidth(360);
    QFormLayout* form = new QFormLayout(&dlg);

    QLineEdit* nameEdit = new QLineEdit(&dlg);
    QComboBox* typeCombo = new QComboBox(&dlg);
    typeCombo->addItems({"灯光", "空调", "窗帘", "摄像头"});
    QComboBox* grpCombo = new QComboBox(&dlg);
    grpCombo->addItems({"客厅", "卧室", "厨房", "门口", "其他"});
    grpCombo->setEditable(true);
    QLineEdit* ipEdit = new QLineEdit("127.0.0.1", &dlg);
    QSpinBox* portSpin = new QSpinBox(&dlg);
    portSpin->setRange(1024, 65535);
    portSpin->setValue(8080);

    form->addRow("设备名称:", nameEdit);
    form->addRow("设备类型:", typeCombo);
    form->addRow("所在分组:", grpCombo);
    form->addRow("IP地址:", ipEdit);
    form->addRow("端口号:", portSpin);

    QDialogButtonBox* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    form->addRow(bb);
    connect(bb, &QDialogButtonBox::accepted, &dlg, [&] {
        if (nameEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(&dlg, "错误", "设备名称不能为空");
            return;
        }
        if (!DatabaseManager::instance()->addDevice(nameEdit->text().trimmed(), typeCombo->currentText(),
                                                    grpCombo->currentText(), ipEdit->text().trimmed(),
                                                    portSpin->value())) {
            QMessageBox::warning(&dlg, "错误", "添加设备失败");
            return;
        }
        dlg.accept();
    });
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        emit deviceChanged();
        loadDevices();
    }
}

void DeviceControlWidget::onEditDevice() {
    if (m_selectedDeviceId < 0) {
        QMessageBox::information(this, "提示", "请先点击一个设备卡片");
        return;
    }

    auto dev = DatabaseManager::instance()->getDeviceById(m_selectedDeviceId);
    if (dev.isEmpty()) {
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle("编辑设备");
    dlg.setFixedWidth(360);
    QFormLayout* form = new QFormLayout(&dlg);

    QLineEdit* nameEdit = new QLineEdit(dev.value("name").toString(), &dlg);
    QComboBox* typeCombo = new QComboBox(&dlg);
    typeCombo->addItems({"灯光", "空调", "窗帘", "摄像头"});
    typeCombo->setCurrentText(dev.value("type").toString());
    QComboBox* grpCombo = new QComboBox(&dlg);
    grpCombo->addItems({"客厅", "卧室", "厨房", "门口", "其他"});
    grpCombo->setEditable(true);
    grpCombo->setCurrentText(dev.value("grp").toString());

    form->addRow("设备名称:", nameEdit);
    form->addRow("设备类型:", typeCombo);
    form->addRow("所在分组:", grpCombo);

    QDialogButtonBox* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    form->addRow(bb);
    connect(bb, &QDialogButtonBox::accepted, &dlg, [&] {
        if (!DatabaseManager::instance()->updateDevice(m_selectedDeviceId, nameEdit->text().trimmed(),
                                                       typeCombo->currentText(), grpCombo->currentText())) {
            QMessageBox::warning(&dlg, "错误", "修改设备失败");
            return;
        }
        dlg.accept();
    });
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        emit deviceChanged();
        loadDevices();
    }
}

void DeviceControlWidget::onDeleteDevice() {
    if (m_selectedDeviceId < 0) {
        QMessageBox::information(this, "提示", "请先点击一个设备卡片");
        return;
    }

    auto dev = DatabaseManager::instance()->getDeviceById(m_selectedDeviceId);
    const QString name = dev.value("name").toString();
    if (QMessageBox::question(this, "确认删除", QString("确定删除设备 \"%1\" 吗？").arg(name)) != QMessageBox::Yes) {
        return;
    }

    if (!DatabaseManager::instance()->deleteDevice(m_selectedDeviceId)) {
        QMessageBox::warning(this, "错误", "删除设备失败");
        return;
    }

    m_selectedDeviceId = -1;
    emit deviceChanged();
    loadDevices();
}

void DeviceControlWidget::onRefresh() {
    loadDevices();
}
