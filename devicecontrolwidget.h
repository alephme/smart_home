#pragma once
#include <QWidget>
#include <QString>
#include <QList>
#include <QMap>
#include <QModelIndex>
#include <QPoint>
#include <QVariant>

class QListWidget;
class QListWidgetItem;

class DeviceControlWidget : public QWidget {
    Q_OBJECT
public:
    explicit DeviceControlWidget(const QString& username, QWidget* parent = nullptr);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

signals:
    void deviceChanged();

private slots:
    void onRefresh();
    void onAddDevice();
    void onEditDevice();
    void onDeleteDevice();
    void onCardItemClicked(QListWidgetItem* item);
    void onCardContextMenu(const QPoint& pos);

private:
    void setupUI();
    void loadDevices();
    void addDeviceCard(const QMap<QString, QVariant>& device);
    void onCardClicked(int deviceId);
    void showDeviceControlDialog(const QMap<QString, QVariant>& device);
    bool tryPairDevice(int deviceId, const QMap<QString, QVariant>& device);
    void restoreDeviceOrderFromSetting();
    void persistDeviceOrderToSetting();
    int orderRank(int deviceId) const;
    void moveOrderByIndex(int from, int to);
    static QMap<QString, QString> parseParams(const QString& params);
    static QString buildParams(const QMap<QString, QString>& kv);

    QString m_username;
    int     m_selectedDeviceId = -1;

    QList<QMap<QString, QVariant>> m_devices;
    QList<int> m_deviceOrder;
    QPoint m_pressPos;
    int m_dragRow = -1;
    bool m_cardDragging = false;
    bool m_suppressCardClick = false;
    class QLabel* m_dragGhost = nullptr;
    QPoint m_dragHotspot;

    class QComboBox*    m_filterCombo = nullptr;
    class QListWidget*  m_cardList    = nullptr;
};
