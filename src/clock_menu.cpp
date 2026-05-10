#include "clock_menu.h"
#include <QDateTime>
#include <QStringListModel>
#include <QMessageBox>
#include <QBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>

ClockMenu::ClockMenu(QWidget* parent)
    : QWidget(parent)
{
    this->setWindowTitle(tr("闹钟"));
    this->setWindowIcon(QIcon(":/images/icon.png"));
    this->setFixedSize(300, 420);
    this->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);

    QDateTime currentTime = QDateTime::currentDateTime();
    currentTime = currentTime.addSecs(60);

    m_dateEdit = new QDateEdit(nullptr);
    m_dateEdit->setDisplayFormat("yyyy-MM-dd");
    m_dateEdit->setDate(currentTime.date());
    m_dateEdit->setCalendarPopup(true);
    
    m_timeEdit = new QTimeEdit(nullptr);
    m_timeEdit->setDisplayFormat("hh::mm::ss");
    m_timeEdit->setTime(currentTime.time());

    m_addBtn = new QPushButton("添加时间", nullptr);
    connect(m_addBtn, &QPushButton::clicked, this, QOverload<>::of(&ClockMenu::addToList));
    m_modifyBtn = new QPushButton("修改时间", nullptr);
    connect(m_modifyBtn, &QPushButton::clicked, this, &ClockMenu::modifyInList);
    m_cancelModifyBtn = new QPushButton("取消修改", nullptr);
    connect(m_cancelModifyBtn, &QPushButton::clicked, this, &ClockMenu::cancelModify);

    m_addBtns = new QWidget(nullptr);
    QHBoxLayout* addBtnsLayout = new QHBoxLayout(nullptr);
    addBtnsLayout->addWidget(m_addBtn);
    m_addBtns->setLayout(addBtnsLayout);

    m_modifyBtns = new QWidget(nullptr);
    QHBoxLayout* modifyBtnsLayout = new QHBoxLayout(nullptr);
    modifyBtnsLayout->addWidget(m_modifyBtn);
    modifyBtnsLayout->addWidget(m_cancelModifyBtn);
    m_modifyBtns->setLayout(modifyBtnsLayout);

    m_Btns = new QStackedWidget(nullptr);
    m_Btns->addWidget(m_addBtns);
    m_Btns->addWidget(m_modifyBtns);
    m_Btns->setCurrentWidget(m_addBtns);

    m_alarmList = AVLTree<QDateTime>();

    m_alarmListModel = new QStringListModel(this);
    QStringList alarmList;
    m_alarmListModel->setStringList(alarmList);
    m_alarmListView = new QListView(nullptr);
    m_alarmListView->setModel(m_alarmListModel);
    m_alarmListView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    this->loadSettings();

    connect(m_alarmListView, &QListView::clicked, this, [this](const QModelIndex& index) {
        if (index.isValid()) {
            QDateTime targetDateTime = QDateTime::fromString((m_alarmListModel->data(index, Qt::DisplayRole)).toString(), "yyyy-MM-dd hh:mm:ss");
            m_dateEdit->setDate(targetDateTime.date());
            m_timeEdit->setTime(targetDateTime.time());
            m_toBeErasedDateTime = targetDateTime;
            m_Btns->setCurrentWidget(m_modifyBtns);
        }
    });

    QFormLayout* formLayout = new QFormLayout();
    formLayout->addRow("日期: ", m_dateEdit);
    formLayout->addRow("时间: ", m_timeEdit);

    QVBoxLayout* centralLayout = new QVBoxLayout(this);
    centralLayout->setContentsMargins(20, 20, 20, 20);
    centralLayout->setSpacing(12);
    
    centralLayout->addLayout(formLayout);
    centralLayout->addWidget(m_Btns);
    
    QLabel* listLabel = new QLabel("已设置的闹钟:");
    centralLayout->addWidget(listLabel);
    
    centralLayout->addWidget(m_alarmListView);
}

ClockMenu::~ClockMenu() {}

void ClockMenu::addToList() {
    QDate targetDate = m_dateEdit->date();
    QTime targetTime = m_timeEdit->time();
    QDateTime targetDateTime(targetDate, targetTime);

    if (targetDateTime <= QDateTime::currentDateTime()) {
        QMessageBox::warning(this,
                        tr("警告"),
                        tr("该时间发生在过去，请更改"),
                        QMessageBox::Ok,
                        QMessageBox::Ok);
        return;
    }
    
    if (m_alarmList.contains(targetDateTime)) {
        QMessageBox::warning(this,
                        tr("警告"),
                        tr("已存在相同时间，请更改"),
                        QMessageBox::Ok,
                        QMessageBox::Ok);
        return;
    }
        
    m_alarmList.insert(targetDateTime);
    int rank = m_alarmList.rank(targetDateTime);
    m_alarmListModel->insertRow(rank);
    m_alarmListModel->setData(m_alarmListModel->index(rank), targetDateTime.toString("yyyy-MM-dd hh:mm:ss"));
    m_alarmListView->update();

    this->saveSettings();
    emit addClock(targetDateTime);
}
void ClockMenu::addToList(QDateTime targetDateTime) {
    if (m_alarmList.contains(targetDateTime)) {
        QMessageBox::warning(this,
                        tr("警告"),
                        tr("已存在相同时间，请更改"),
                        QMessageBox::Ok,
                        QMessageBox::Ok);
        return;
    }
        
    m_alarmList.insert(targetDateTime);
    int rank = m_alarmList.rank(targetDateTime);
    m_alarmListModel->insertRow(rank);
    m_alarmListModel->setData(m_alarmListModel->index(rank), targetDateTime.toString("yyyy-MM-dd hh:mm:ss"));

    this->saveSettings();
    emit addClock(targetDateTime);
}

void ClockMenu::cancelClock(QDateTime targetDateTime) {
    int rank = m_alarmList.rank(targetDateTime);
    m_alarmList.erase(targetDateTime);
    m_alarmListModel->removeRow(rank);
    this->saveSettings();
    emit cancelClock(rank);
}
void ClockMenu::modifyInList() {
    this->cancelClock(m_toBeErasedDateTime);
    this->addToList();
    this->cancelModify();
}
void ClockMenu::cancelModify() {
    m_toBeErasedDateTime = QDateTime();
    m_Btns->setCurrentWidget(m_addBtns);
}

void ClockMenu::scheduleNext(QDateTime targetDateTime) {
    this->cancelClock(targetDateTime);
    targetDateTime = targetDateTime.addDays(1);
    addToList(targetDateTime);
}

void ClockMenu::loadSettings() {
    QFile file("alarms.json");
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isArray()) {
            QJsonArray arr = doc.array();
            for (const QJsonValue& v : arr) {
                QString dtStr = v.toString();
                QDateTime dt = QDateTime::fromString(dtStr, "yyyy-MM-dd hh:mm:ss");
                if (dt.isValid() && dt > QDateTime::currentDateTime()) {
                    // Check if it already exists before adding
                    if (!m_alarmList.contains(dt)) {
                        m_alarmList.insert(dt);
                        int rank = m_alarmList.rank(dt);
                        m_alarmListModel->insertRow(rank);
                        m_alarmListModel->setData(m_alarmListModel->index(rank), dt.toString("yyyy-MM-dd hh:mm:ss"));
                        emit addClock(dt);
                    }
                }
            }
        }
    }
}

void ClockMenu::saveSettings() {
    QJsonArray arr;
    QStringList strList = m_alarmListModel->stringList();
    for (const QString& str : strList) {
        arr.append(str);
    }
    QJsonDocument doc(arr);
    QFile file("alarms.json");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
    }
}