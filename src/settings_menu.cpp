#include "settings_menu.h"
#include <QString>
#include <QStringBuilder>
#include <QFile>
#include <QVariant>

#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonValueRef>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>

#include <QMessageBox>

constexpr uint scaleModifed = 1;
constexpr uint foodSpawnIntervalModifed = 2;
constexpr uint enableAIModifed = 4;

SettingsMenu::SettingsMenu(bool init, QWidget* parent)
    : QDialog(parent), m_init(init)
{   
    m_unsavedWindowTitle = QLatin1String("Settings*") % (m_init ? " 第一次进入需要设置" : "");
    m_savedWindowTitle = QLatin1String("Settings") % (m_init ? " 第一次进入需要设置" : "");
    this->setWindowTitle(m_unsavedWindowTitle);
    this->setWindowIcon(QIcon(":/images/icon.png"));
    this->setFixedSize(400, 500);
    this->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);

    m_scaleSlider = new QSlider(Qt::Horizontal, nullptr);
    m_scaleSlider->setRange(5, 30);
    m_scaleSlider->setTickPosition(QSlider::TicksBelow);
    m_scaleSlider->setTickInterval(5);
    m_scaleText = new QLabel(nullptr);

    m_foodSpawnIntervalSpinBox = new QSpinBox(nullptr);
    m_foodSpawnIntervalSpinBox->setRange(1, 10000);

    m_enableAICheckBox = new QCheckBox("启用 AI", nullptr);

    m_saveBtn = new QPushButton("保存", nullptr);

    connect(m_scaleSlider, &QSlider::valueChanged, this, [this](int value) {
        double scale = value / 10.0;
        m_scaleText->setText(QString::number(scale));
        if (!m_prevScale.isNull() && m_prevScale.toInt() == value) {
            m_hasModified &= ~scaleModifed;
        } else {
            m_hasModified |= scaleModifed;
        }
        emit settingsChanged(m_hasModified);
        emit scaleValueChanged(scale);
    });

    connect(m_foodSpawnIntervalSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
        if (!m_prevFoodSpawnInterval.isNull() && m_prevFoodSpawnInterval.toInt() == value) {
            m_hasModified &= ~foodSpawnIntervalModifed;
        } else {
            m_hasModified |= foodSpawnIntervalModifed;
        }
        emit settingsChanged(m_hasModified);
    });

    connect(m_enableAICheckBox, &QCheckBox::toggled, this, [this](bool state) {
        if (!m_prevEnableAI.isNull() && m_prevEnableAI.toBool() == state) {
            m_hasModified &= ~enableAIModifed;
        } else {
            m_hasModified |= enableAIModifed;
        }
        emit settingsChanged(m_hasModified);
    });
    
    connect(m_saveBtn, &QPushButton::clicked, this, &SettingsMenu::saveSettings);

    connect(this, &SettingsMenu::settingsChanged, this, [this](int state) {
        if (state) {
            this->setWindowTitle(m_unsavedWindowTitle);
        } else {
            this->setWindowTitle(m_savedWindowTitle);
        }
    });

    m_hasModified = scaleModifed | foodSpawnIntervalModifed | enableAIModifed;

    m_scaleSlider->setValue(10);
    m_scaleText->setText("1.0");
    m_foodSpawnIntervalSpinBox->setValue(60);
    m_enableAICheckBox->setChecked(false);

    this->loadSettings();

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    QFormLayout* formLayout = new QFormLayout();
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    
    QHBoxLayout* scaleLayout = new QHBoxLayout();
    scaleLayout->addWidget(m_scaleSlider);
    scaleLayout->addWidget(m_scaleText);
    formLayout->addRow("缩放比例:", scaleLayout);
    
    formLayout->addRow("食物产出间隔(秒):", m_foodSpawnIntervalSpinBox);
    formLayout->addRow("其他选项:", m_enableAICheckBox);

    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(m_saveBtn);
    mainLayout->addLayout(btnLayout);
}

SettingsMenu::~SettingsMenu() {}

void SettingsMenu::closeEvent(QCloseEvent* event) {
    if (m_hasModified) {
        QMessageBox::StandardButton reply;
        QString msg = QStringLiteral("设置已被修改，是否保存？") % (m_init ? "\n第一次不设置会导致程序直接退出" : "");
        reply = QMessageBox::question(this, "保存修改", msg,
                                      QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (reply == QMessageBox::Save) {
            saveSettings();
            this->accept();
            event->accept();
        } else if (reply == QMessageBox::Discard) {
            this->reject();
            event->accept();
        } else {
            event->ignore();
        }
    } else {
        event->accept();
    }
}

void SettingsMenu::loadSettings() {
    QFile file("config.json");
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QString config = file.readAll();
        file.close();
        
        QJsonParseError err_rpt;
        QJsonDocument root_document = QJsonDocument::fromJson(config.toUtf8(), &err_rpt);
        if (err_rpt.error == QJsonParseError::NoError) {
            QJsonObject Config = root_document.object();
            if (Config.contains("scale")) {
                m_hasModified &= ~scaleModifed;
                m_prevScale.setValue(Config["scale"].toInt());
                m_scaleSlider->setValue(Config["scale"].toInt());
            }
            if (Config.contains("foodSpawnInterval")) {
                m_hasModified &= ~foodSpawnIntervalModifed;
                m_prevFoodSpawnInterval.setValue(Config["foodSpawnInterval"].toInt());
                m_foodSpawnIntervalSpinBox->setValue(Config["foodSpawnInterval"].toInt());
            }
            if (Config.contains("enableAI")) {
                m_hasModified &= ~enableAIModifed;
                m_prevEnableAI.setValue(Config["enableAI"].toBool());
                m_enableAICheckBox->setChecked(Config["enableAI"].toBool());
            }
            emit settingsChanged(m_hasModified);
        }
    }
}

void SettingsMenu::saveSettings() {
    m_hasModified = 0;
    this->setWindowTitle(m_savedWindowTitle);

    m_prevScale.setValue(m_scaleSlider->value());
    m_prevFoodSpawnInterval.setValue(m_foodSpawnIntervalSpinBox->value());
    m_prevEnableAI.setValue(m_enableAICheckBox->isChecked());

    QJsonObject Config;
    Config["scale"] = m_scaleSlider->value();
    Config["foodSpawnInterval"] = m_foodSpawnIntervalSpinBox->value();
    Config["enableAI"] = m_enableAICheckBox->isChecked();

    QJsonDocument doc(Config);
    QByteArray jsonData = doc.toJson(QJsonDocument::Indented);

    QFile file("config.json");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(jsonData);
        file.close();
    }

    if (m_init) {
        this->close();
    }
}