#pragma once
#include <QWidget>
#include <QString>
#include <QList>

class HomeWidget : public QWidget {
    Q_OBJECT
public:
    explicit HomeWidget(const QString& username, QWidget* parent = nullptr);

signals:
    void navigateTo(int pageIndex);

public slots:
    void refresh();

private:
    void setupUI();
    void updateStats();
    void refreshFavoriteScenes();
    void openFavoriteSceneEditor();
    void activateFavoriteScene(int sceneId);
    QList<int> loadFavoriteSceneIds() const;
    void saveFavoriteSceneIds(const QList<int>& ids) const;
    QString m_username;

    class QLabel* m_onlineLbl  = nullptr;
    class QLabel* m_offlineLbl = nullptr;
    class QLabel* m_alarmLbl   = nullptr;
    class QLabel* m_timeLbl    = nullptr;
    class QTimer* m_timer      = nullptr;
    class QHBoxLayout* m_sceneLay = nullptr;
};
