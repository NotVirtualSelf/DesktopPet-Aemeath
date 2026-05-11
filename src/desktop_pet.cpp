#include "desktop_pet.h"
#include <QIcon>
#include <QLabel>
#include <QPixmap>
#include <QVBoxLayout>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QMenu>
#include <QUrl>
#include <QRandomGenerator>

#include "settings_menu.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonValueRef>

#include <QMessageBox>
#include <QInputDialog>
#include <QScreen>
#include <QGuiApplication>
#include <QNetworkRequest>
#include <QHttpMultiPart>
#include <QBuffer>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

constexpr int WINDOW_WIDTH = 260;
constexpr int WINDOW_HEIGHT = 227;

const char* OCR_apiKey = "sk-5d5608f668ad455bb1d2d1f32c9a51c5";

DesktopPet::DesktopPet(QWidget* parent)
    : QWidget(parent),
    m_Scale(1.0), m_dragPosition(QPoint(0, 0)), m_isDragging(false),
    m_Food(0), m_musicPlayer(nullptr), m_chatDialog(nullptr),
    m_targetProcessRunning(false), m_isMovedToCorner(false), m_originalPosition(QPoint())
{
    this->setWindowTitle("Aemeath");
    this->setWindowIcon(QIcon(":/images/icon.png"));
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    this->setAttribute(Qt::WA_TranslucentBackground);

    QFile file("config.json");
    if (!QFile::exists("config.json")) {
        this->showSettings(true);
    }
    
    QLabel* stand = new QLabel(this);
    stand->setPixmap(QPixmap(":/images/stand.png"));
    stand->setScaledContents(true);
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(stand, Qt::AlignCenter);
    
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &DesktopPet::customContextMenuRequested, this, &DesktopPet::showMenu);
    
    m_FoodSpawn = new QTimer(this);
    m_FoodSpawn->setTimerType(Qt::PreciseTimer);
    connect(m_FoodSpawn, &QTimer::timeout, this, &DesktopPet::foodInc);

    if (QFile::exists("config.json")) {
        this->applySettings();
    } else {
        exit(1);
    }

    m_clockMenu = new ClockMenu(this);
    m_clockMenu->hide();

    connect(m_clockMenu, &ClockMenu::addClock, this, &DesktopPet::addClock);
    connect(m_clockMenu, QOverload<int>::of(&ClockMenu::cancelClock), this, &DesktopPet::cancelClock);

    // Initialize process monitor
    m_processMonitorTimer = new QTimer(this);
    connect(m_processMonitorTimer, &QTimer::timeout, this, &DesktopPet::checkProcessStatus);
    m_processMonitorTimer->start(5000); // Check every 5 seconds

    m_networkManager = new QNetworkAccessManager(this);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &DesktopPet::handleOcrResponse);
}

DesktopPet::~DesktopPet() {}

void DesktopPet::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        m_isDragging = true;
    }
    event->accept();
}

void DesktopPet::mouseMoveEvent(QMouseEvent* event) {
    if (m_isDragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_dragPosition);
    }
    event->accept();
}

void DesktopPet::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
    }
    event->accept();
}

void DesktopPet::showMenu(const QPoint& pos) {
    QMenu menu;
    menu.addAction(std::format("投喂(零食数量：{})", m_Food).c_str(), this, &DesktopPet::feed);
    if (m_enableAI) {
        menu.addAction("AI对话", this, &DesktopPet::aiChat);
    }
    menu.addAction("闹钟", this, &DesktopPet::clock);
    menu.addAction("唱歌", this, &DesktopPet::sing);
    menu.addAction("跳舞", this, &DesktopPet::dance);
    menu.addAction("设置", this, &DesktopPet::settings);

    menu.exec(this->mapToGlobal(pos));
}

void DesktopPet::settings() {
    this->showSettings();
    this->applySettings();
}

void DesktopPet::showSettings(bool init) {
    SettingsMenu menu(init, this);
    connect(&menu, &SettingsMenu::scaleValueChanged, this, [this](double scale) {
        m_Scale = scale;
        this->setFixedSize(WINDOW_WIDTH * m_Scale, WINDOW_HEIGHT * m_Scale);
    });
    menu.exec();
}

void DesktopPet::applySettings() {
    QFile file("config.json");
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QString config = file.readAll();
        file.close();
        
        QJsonParseError err_rpt;
        QJsonDocument root_document = QJsonDocument::fromJson(config.toUtf8(), &err_rpt);
        if (err_rpt.error == QJsonParseError::NoError) {
            QJsonObject Config = root_document.object();
            if (Config.contains("scale")) {
                double scale = Config["scale"].toInt() / 10.0;
                m_Scale = scale;
                this->setFixedSize(WINDOW_WIDTH * m_Scale, WINDOW_HEIGHT * m_Scale);
            }
            if (Config.contains("foodSpawnInterval")) {
                m_FoodSpawnInterval = Config["foodSpawnInterval"].toInt();
                m_FoodSpawn->start(m_FoodSpawnInterval * 1000);
            }
            if (Config.contains("enableAI")) {
                m_enableAI = Config["enableAI"].toBool();
            }
        }
    }
}

void DesktopPet::aiChat() {
    if (!m_chatDialog) {
        m_chatDialog = new AIChatDialog(this);
    }
    m_chatDialog->show();
    m_chatDialog->raise();
    m_chatDialog->activateWindow();
}

void DesktopPet::feed() {
    if (m_Food) {
        --m_Food;
        QSoundEffect* effect = new QSoundEffect(this);
        effect->setSource(QUrl::fromLocalFile(":/sounds/happy.wav"));
        effect->play();
        connect(
            effect, 
            &QSoundEffect::playingChanged, 
            [effect]() {
                if (!effect->isPlaying()) effect->deleteLater();
            }
        );
        this->dance();
    }
}

void DesktopPet::foodInc() {
    ++m_Food;
}

void DesktopPet::clock() {
    m_clockMenu->show();
}

void DesktopPet::addClock(QDateTime targetDateTime) {
    QTimer* newTimer = new QTimer(nullptr);
    qint64 newTimeStamp = targetDateTime.toSecsSinceEpoch();
    int msecs = QDateTime::currentDateTime().msecsTo(targetDateTime);
    newTimer->setSingleShot(true);
    newTimer->start(msecs);
    connect(newTimer, &QTimer::timeout, this, [this, targetDateTime]() {
        clockTimeout();
        if (QMessageBox::warning(this,
                        tr("闹钟"),
                        tr("设置的时间到了！是否设置下一天的闹钟？"),
                        QMessageBox::Ok | QMessageBox::No,
                        QMessageBox::Ok)) {
            m_clockMenu->scheduleNext(targetDateTime);
        } else {
            m_clockMenu->cancelClock(targetDateTime);
        }
        
    });
    m_timer.insert(newTimeStamp);
    m_Stamp2Timer[newTimeStamp] = newTimer;
}

void DesktopPet::cancelClock(int rank) {
    qint64 oldTimeStamp = m_timer.kth(rank);
    m_timer.erase(oldTimeStamp);

    QTimer* oldTimer = m_Stamp2Timer[oldTimeStamp];
    delete oldTimer;
    m_Stamp2Timer.erase(oldTimeStamp);
}

void DesktopPet::clockTimeout() {
    QSoundEffect* effect = new QSoundEffect(this);
    effect->setSource(QUrl::fromLocalFile(":/sounds/clock.wav"));
    effect->play();
    connect(
        effect, 
        &QSoundEffect::playingChanged, 
        [effect]() {
            if (!effect->isPlaying()) effect->deleteLater();
        }
    );
}

void DesktopPet::sing() {
    if (m_musicPlayer) {
        m_musicPlayer->stop();
        m_musicPlayer->deleteLater();
    }
    m_musicPlayer = new QSoundEffect(this);
    QStringList songs = {
        ":/music/1.wav",
        ":/music/2.wav",
        ":/music/3.wav",
        ":/music/4.wav",
        ":/music/那颗星梦见的春日.wav"
    };
    int index = QRandomGenerator::global()->bounded(songs.size());
    m_musicPlayer->setSource(QUrl::fromLocalFile(songs[index]));
    m_musicPlayer->play();
}

void DesktopPet::dance() {
    QLabel* label = this->findChild<QLabel*>();
    if (!label) return;

    if (label->property("isDancing").toBool()) {
        return;
    }
    label->setProperty("isDancing", true);

    QTimer* danceTimer = new QTimer(this);
    
    connect(danceTimer, &QTimer::timeout, this, [label, danceTimer, frame = 1]() mutable {
        if (frame > 11) {
            label->setPixmap(QPixmap(":/images/stand.png"));
            label->setProperty("isDancing", false);
            danceTimer->stop();
            danceTimer->deleteLater();
            return;
        }
        label->setPixmap(QPixmap(QString(":/images/dance%1.png").arg(frame)));
        frame++;
    });
    
    danceTimer->start(100);
}

void DesktopPet::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_targetProcessRunning) {
        this->processScreenshot();
    }
    QWidget::mouseDoubleClickEvent(event);
}

void DesktopPet::checkProcessStatus() {
#ifdef Q_OS_WIN
    QProcess process;
    process.start("tasklist", QStringList() << "/FI" << "IMAGENAME eq target_game.exe"); // Replace target_game.exe with your target process
    process.waitForFinished();
    QString output(process.readAllStandardOutput());
    bool isRunning = output.contains("target_game.exe", Qt::CaseInsensitive);
    
    if (isRunning && !m_isMovedToCorner) {
        m_originalPosition = this->pos();
        QScreen* screen = QGuiApplication::primaryScreen();
        if (screen) {
            QRect rect = screen->availableGeometry();
            this->move(rect.right() - this->width(), rect.top());
        }
        m_isMovedToCorner = true;
    } else if (!isRunning && m_isMovedToCorner) {
        this->move(m_originalPosition);
        m_isMovedToCorner = false;
    }
    
    m_targetProcessRunning = isRunning;
#endif
    // If on Mac/Linux, similar commands like `pgrep` can be used.
}

void DesktopPet::processScreenshot() {
    QScreen* screen = QGuiApplication::primaryScreen();
    if (!screen) return;
    
    WId windowId = 0;
#ifdef Q_OS_WIN
    HWND hwnd = FindWindowW(nullptr, L"Target Game Window Title");
    if (hwnd) {
        windowId = reinterpret_cast<WId>(hwnd);
    }
#endif
    
    QPixmap originalPixmap = screen->grabWindow(windowId);
    QString fileName = QDir::currentPath() + "/screenshot_temp.jpg";
    originalPixmap.save(fileName, "JPG", 80);
    
    // 1. Run YOLO placeholder
    QProcess* yoloProcess = new QProcess(this);
    connect(yoloProcess, &QProcess::finished, this, [fileName, yoloProcess]() {
        // Placeholder for YOLO output parsing etc
        qDebug() << "YOLO finished:" << yoloProcess->readAllStandardOutput();
        yoloProcess->deleteLater();
        
        // Ensure file is deleted if OCR won't be called, but here we call OCR on the file as well.
        // QFile::remove(fileName); 
    });
    // Replace placeholder path with your actual python script and parameters
    // yoloProcess->start("python", QStringList() << "run_yolo.py" << fileName); 
    
    // 2. OCR API using DashScope qwen-vl-ocr
    QUrl ocrUrl("https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions"); 
    QNetworkRequest request(ocrUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QByteArray("Bearer ") + OCR_apiKey);

    // 将图片转换为 base64
    QByteArray imageByteArray;
    QBuffer buffer(&imageByteArray);
    buffer.open(QIODevice::WriteOnly);
    originalPixmap.save(&buffer, "PNG");
    QString base64Image = QByteArray("data:image/png;base64,") + imageByteArray.toBase64();

    QJsonObject textContentObj;
    textContentObj["type"] = "text";
    // 针对文字成块状分布的情况，通过优化提示词（Prompt），要求模型按版式或结构化输出
    textContentObj["text"] = "请提取图片中的所有文字。由于图片中文字成块状分布，请严格按照图片上的视觉排版和版块结构进行格式化输出（可使用Markdown格式），不同区块的文字请分段给出，保持原有的阅读逻辑。";
    
    QJsonObject imageUrlObj;
    imageUrlObj["url"] = base64Image;
    QJsonObject imageContentObj;
    imageContentObj["type"] = "image_url";
    imageContentObj["image_url"] = imageUrlObj;

    QJsonArray contentArray;
    contentArray.append(imageContentObj);
    contentArray.append(textContentObj);

    QJsonObject messageObj;
    messageObj["role"] = "user";
    messageObj["content"] = contentArray;

    QJsonArray messagesArray;
    messagesArray.append(messageObj);

    QJsonObject requestObj;
    requestObj["model"] = "qwen-vl-ocr"; // 指定 qwen-vl-ocr
    requestObj["messages"] = messagesArray;

    QJsonDocument doc(requestObj);
    QByteArray postData = doc.toJson();

    QNetworkReply* reply = m_networkManager->post(request, postData);
    reply->setProperty("fileName", fileName); // store file name to delete it later
}

void DesktopPet::handleOcrResponse(QNetworkReply* reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(response);
        QJsonObject jsonObj = jsonResponse.object();
        
        if (jsonObj.contains("choices")) {
            QJsonArray choices = jsonObj["choices"].toArray();
            if (!choices.isEmpty()) {
                QJsonObject choiceObj = choices[0].toObject();
                QJsonObject messageObj = choiceObj["message"].toObject();
                QString content = messageObj["content"].toString();
                qDebug() << "OCR result content:" << content;
                // 这里可以把识别结果传递给 AI 对话系统，或做进一步处理
            }
        } else {
            qDebug() << "Unexpected JSON format:" << response;
        }
    } else {
        qDebug() << "OCR error:" << reply->errorString();
        qDebug() << "Error detail:" << reply->readAll();
    }
    
    QString fileName = reply->property("fileName").toString();
    if (!fileName.isEmpty()) {
        QFile::remove(fileName);
    }
    
    reply->deleteLater();
}