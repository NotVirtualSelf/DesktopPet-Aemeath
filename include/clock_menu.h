#ifndef CLOCK_MENU_H
#define CLOCK_MENU_H

#include <QWidget>
#include <QDateEdit>
#include <QTimeEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QStackedWidget>
#include "AVLTree.h"
#include <QStringListModel>
#include <QListView>

class ClockMenu : public QWidget {
    Q_OBJECT

public:
    explicit ClockMenu(QWidget* parent = nullptr);
    ~ClockMenu();
    void cancelClock(QDateTime targetDateTime);
    void scheduleNext(QDateTime targetDateTime);

private:
    bool m_modified;
    QDateEdit* m_dateEdit;
    QTimeEdit* m_timeEdit;
    QPushButton* m_addBtn;
    QWidget* m_addBtns;
    QPushButton* m_modifyBtn;
    QPushButton* m_cancelModifyBtn;
    QWidget* m_modifyBtns;
    QStackedWidget* m_Btns;
    AVLTree<QDateTime> m_alarmList;
    QStringListModel* m_alarmListModel;
    QListView* m_alarmListView;
    QDateTime m_toBeErasedDateTime;

    void loadSettings();
    void saveSettings();
    void addToList(QDateTime targetDateTime);

private slots:
    void addToList();
    void modifyInList();
    void cancelModify();

signals:
    void addClock(QDateTime thisDateTime);
    void cancelClock(int rank);
};

#endif