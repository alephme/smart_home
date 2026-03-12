#pragma once
#include <QWidget>

class SettingsWidget : public QWidget {
    Q_OBJECT
public:
    explicit SettingsWidget(QWidget* parent = nullptr);

private slots:
    void onSaveSettings();

private:
    void setupUI();
    void applyTheme(const QString& themeName);  // 新增：应用主题

    class QComboBox* m_themeCombo = nullptr;
    class QSpinBox* m_refreshSpin = nullptr;
    class QLineEdit* m_dbPathEdit = nullptr;
};
