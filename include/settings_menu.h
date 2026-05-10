#ifndef SETTINGS_MENU_H
#define SETTINGS_MENU_H

#include <QDialog>
#include <QString>
#include <QSlider>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QVariant>
#include <QCloseEvent>
#include <QPushButton>

class SettingsMenu : public QDialog {
    Q_OBJECT

public:
    SettingsMenu(bool init = false, QWidget* parent = nullptr);
    ~SettingsMenu();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    bool m_init;
    QString m_unsavedWindowTitle, m_savedWindowTitle;
    QSlider* m_scaleSlider;
    QLabel* m_scaleText;
    QSpinBox* m_foodSpawnIntervalSpinBox;
    QCheckBox* m_enableAICheckBox;
    QPushButton* m_saveBtn;

    QVariant m_prevScale;
    QVariant m_prevFoodSpawnInterval;
    QVariant m_prevEnableAI;
    uint m_hasModified;

    void loadSettings();
    void saveSettings();

signals:
    void scaleValueChanged(double value);
    void settingsChanged(int state);
};

#endif