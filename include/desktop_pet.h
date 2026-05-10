#ifndef DESKTOP_PET_H
#define DESKTOP_PET_H

#include <QWidget>
#include <QPoint>
#include <QMouseEvent>
#include <QTimer>
#include <QDateTime>
#include <QSoundEffect>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "clock_menu.h"
#include "ai_chat_dialog.h"

class DesktopPet : public QWidget {
    Q_OBJECT

public:
    DesktopPet(QWidget *parent = nullptr);
    ~DesktopPet();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    double m_Scale;
    QPoint m_dragPosition;
    bool m_isDragging;
    int m_FoodSpawnInterval;
    QTimer* m_FoodSpawn;
    int m_Food;
    bool m_enableAI;
    bool m_isSetClock;
    QDateTime m_alarmTime;
    ClockMenu* m_clockMenu;
    QSoundEffect* m_musicPlayer;
    
    void feed();
    void sing();
    void dance();
    void aiChat();
    void showSettings(bool init = false);
    void applySettings();

    AIChatDialog* m_chatDialog;
    AVLTree<qint64> m_timer;
    std::unordered_map<qint64, QTimer*> m_Stamp2Timer;

    // Process monitoring & OCR/YOLO
    QTimer* m_processMonitorTimer;
    bool m_targetProcessRunning;
    QPoint m_originalPosition;
    bool m_isMovedToCorner;
    QNetworkAccessManager* m_networkManager;
    void processScreenshot();

private slots:
    void showMenu(const QPoint& pos);
    void foodInc();
    void settings();
    void clock();
    void addClock(QDateTime targetDateTime);
    void cancelClock(int rank);
    void clockTimeout();
    void checkProcessStatus();
    void handleOcrResponse(QNetworkReply* reply);
};

#endif