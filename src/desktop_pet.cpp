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

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>

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
#include <QTextBrowser>
#include <QPushButton>
#include "native_web_view.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

constexpr int WINDOW_WIDTH = 260;
constexpr int WINDOW_HEIGHT = 227;

constexpr const char* OCR_apiKey = "";
constexpr const char* chat_apiKey = "";

DesktopPet::DesktopPet(QWidget* parent)
    : QWidget(parent),
    m_Scale(1.0), m_dragPosition(QPoint(0, 0)), m_isDragging(false),
    m_Food(0), m_musicPlayer(nullptr), m_chatDialog(nullptr),
    m_targetProcessRunning(false), m_isMovedToCorner(false), m_originalPosition(QPoint()),
    m_activeBubble(nullptr), m_activeBubbleText(nullptr), m_activeBubbleInnerLayout(nullptr),
    m_activeBubbleContent(nullptr),
    m_activeBubbleHasWebView(false)
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

    m_processMonitorTimer = new QTimer(this);
    connect(m_processMonitorTimer, &QTimer::timeout, this, &DesktopPet::checkProcessStatus);
    m_processMonitorTimer->start(3000);

    m_networkManager = new QNetworkAccessManager(this);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &DesktopPet::handleResponse);
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
        this->showLoadingBubble();
        this->processScreenshot();
    }
    QWidget::mouseDoubleClickEvent(event);
}

void DesktopPet::checkProcessStatus() {
#ifdef Q_OS_WIN
    QProcess process;
    process.start("tasklist", QStringList() << "/FI" << "IMAGENAME eq Wuthering Waves.exe"); // Replace target_game.exe with your target process
    process.waitForFinished();
    QString output(process.readAllStandardOutput());
    bool isRunning = output.contains("Wuthering Waves.exe", Qt::CaseInsensitive);
    
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
    HWND hwnd = FindWindowW(nullptr, L"Wuthering Waves");
    if (hwnd) {
        windowId = reinterpret_cast<WId>(hwnd);
    }
#endif
    
    QPixmap originalPixmap = screen->grabWindow(windowId);
    QString fileName = QDir::currentPath() + "/screenshot_temp.jpg";
    originalPixmap.save(fileName, "JPG", 80);
    
    // 1. Run YOLO (OpenCV DNN C++ Inference)
    QString modelPath = QDir::currentPath() + "/yolo/best.onnx";
    cv::dnn::Net net = cv::dnn::readNetFromONNX(modelPath.toStdString());
    
    if (net.empty()) {
        qDebug() << "Failed to load YOLO ONNX model from:" << modelPath;
    } else {
        cv::Mat img = cv::imread(fileName.toStdString());
        cv::Mat blob;
        cv::dnn::blobFromImage(img, blob, 1.0 / 255.0, cv::Size(640, 640), cv::Scalar(), true, false);
        
        net.setInput(blob);
        std::vector<cv::Mat> outputs;
        net.forward(outputs, net.getUnconnectedOutLayersNames());
        
        // YOLOv8 output size is usually [1, 4+num_classes, 8400]
        cv::Mat net_output = outputs[0];
        int num_classes = net_output.size[1] - 4;
        int elements = net_output.size[2];
        
        QStringList labels;
        std::vector<QString> classNames = {
            "Character - Rover", "Character - Yangyang", "Character - Chixia", "Character - Baizhi",
            "Character - Changli", "Character - Jinhsi", "Character - Yinlin", "Character - Jiyan",
            "Character - Lingyang", "Character - Verina", "Character - Jianxin", "Character - Taoqi",
            "Character - Yuanwu", "Character - Danjin", "Character - Mortefi", "Character - Sanhua",
            "Character - Zhezhi", "Character - XiangliYao", "Character - Youhu", "Character - Lumi",
            "Character - Encore", "Character - Aalto", "Character - Camellya", "Character - TheShorekeeper",
            "Character - Phoebe", "Character - Brant", "Character - Zani", "Character - CarlottaMontelli",
            "Character - CantarellaFisalia", "Character - CiacconaToccata", "Character - Cartethyia",
            "Character - Lupa", "Character - Calcharo", "Character - Phrolova", "Character - Qiuyuan",
            "Character - Galbrena", "Character - Iuno", "Character - Augusta", "Character - KuchibaChisa",
            "Character - Buling", "Character - Lynae", "Character - Mornye", "Character - Aemeath",
            "Character - LuukHerssen", "Character - Sigrika", "Character - Denia", "Character - Hiyuki",
            "Character - Lucilla", "Character - Roccia", "Other"
        };
        
        for (int i = 0; i < elements; ++i) {
            float maxProb = 0.0f;
            int classId = -1;
            
            // 找出置信度最高的类别
            for (int c = 0; c < num_classes; ++c) {
                float prob = net_output.at<float>(0, 4 + c, i);
                if (prob > maxProb) {
                    maxProb = prob;
                    classId = c;
                }
            }
            
            // 置信度阈值 (可调)
            if (maxProb > 0.5f && classId >= 0 && classId < static_cast<int>(classNames.size())) {
                QString label = classNames[classId];
                if (!labels.contains(label)) {
                    labels.append(label);
                }
            }
        }
        
        qDebug() << "C++ OpenCV DNN Detected Labels:" << labels;
        
        // 筛选出角色标签名称
        QStringList characterNames;
        for (const QString& label : labels) {
            if (label.startsWith("Character - ")) {
                characterNames.append(label.mid(12).trimmed()); // 取出 "Character - " 后的实际名字
            }
        }
        
        if (!characterNames.isEmpty()) {
            qDebug() << "检测到角色:" << characterNames << "，正在请求 AI 与本地数据...";
            
            // 无需截图做 OCR，直接删除
            QFile::remove(fileName);
            
            // 2(a). 调用文本 AI 获取角色基本信息
            QNetworkRequest request(QUrl("https://api.deepseek.com/chat/completions"));
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
            request.setRawHeader("Authorization", QString("Bearer %1").arg(chat_apiKey).toUtf8());

            QJsonObject messageObj;
            messageObj["role"] = "user";
            messageObj["content"] = QString("请简要介绍一下《鸣潮》游戏中的这些角色：%1").arg(characterNames.join(", "));

            QJsonArray messagesArray;
            messagesArray.append(messageObj);

            QJsonObject requestObj;
            requestObj["model"] = "deepseek-v4-flash"; 
            requestObj["messages"] = messagesArray;

            QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(requestObj).toJson());
            reply->setProperty("requestType", "character_ai");
            reply->setProperty("characterNames", characterNames.join(","));
            
        } else {
            qDebug() << "未检测到角色，由于图片内容包含其他元素，即将进行 OCR 识别...";
            // 2(b). 没有角色时调用 OCR API using DashScope qwen-vl-ocr
            QUrl ocrUrl("https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions"); 
            QNetworkRequest request(ocrUrl);
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
            request.setRawHeader("Authorization", QByteArray("Bearer ") + OCR_apiKey);

            // 将图片转换为 base64
            QByteArray imageByteArray;
            QBuffer buffer(&imageByteArray);
            buffer.open(QIODevice::WriteOnly);
            // 重新使用上方已抓取的 originalPixmap
            originalPixmap.save(&buffer, "PNG");
            QString base64Image = QByteArray("data:image/png;base64,") + imageByteArray.toBase64();

            QJsonObject textContentObj;
            textContentObj["type"] = "text";
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
            requestObj["model"] = "qwen-vl-ocr";
            requestObj["messages"] = messagesArray;

            QJsonDocument doc(requestObj);
            QByteArray postData = doc.toJson();

            QNetworkReply* reply = m_networkManager->post(request, postData);
            reply->setProperty("requestType", "ocr");
            reply->setProperty("fileName", fileName); // 供 OCR 完成后删除文件
        }
    }
}

void DesktopPet::handleResponse(QNetworkReply* reply) {
    QString reqType = reply->property("requestType").toString();
    
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
                
                if (reqType == "character_ai") {
                    QString charNamesStr = reply->property("characterNames").toString();
                    QStringList charNames = charNamesStr.split(",");
                    QString targetUrl;
                    
                    // 读取本地数据：原本存文本的 TXT 文件，现在视为存放对应网页的 URL
                    for (const QString& name : charNames) {
                        QFile file(QDir::currentPath() + "/data/" + name + ".txt");
                        if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                            targetUrl = "https://wiki.kurobbs.com/mc/item/" + QString::fromUtf8(file.readAll()).trimmed();
                            file.close();
                            if (!targetUrl.isEmpty()) {
                                break; // 提取到第一个合法 URL 即可停止
                            }
                        }
                    }
                    
                    qDebug().noquote() << "\n" << content;
                    if (!targetUrl.isEmpty()) qDebug().noquote() << "[内置 Web 指向]: " << targetUrl << "\n";
                    showBubble(content, targetUrl);
                                       
                } else if (reqType == "ocr") {
                    qDebug() << "OCR 识别完毕，准备请求 AI 进行游戏内容分析...";
                    QString prompt;
                    if (content.contains("深塔") || content.contains("海墟") || content.contains("海域") || content.contains("湍渊") || content.contains("矩阵")) {
                        prompt = QString("在这张《鸣潮》游戏的截图中，识别到了以下文字：\n%1\n\n请结合《鸣潮》，问一下遇到这种情况，应该怎么进行角色配队？").arg(content);
                    } else {
                        prompt = QString("在这张《鸣潮》游戏的截图中，识别到了以下文字：\n%1\n\n请问这些文字在鸣潮游戏中指的是什么内容？").arg(content);
                    }
                    
                    QNetworkRequest aiReq(QUrl("https://api.deepseek.com/chat/completions"));
                    aiReq.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
                    aiReq.setRawHeader("Authorization", QString("Bearer %1").arg(chat_apiKey).toUtf8());

                    QJsonObject messageObj;
                    messageObj["role"] = "user";
                    messageObj["content"] = prompt;

                    QJsonArray messagesArray;
                    messagesArray.append(messageObj);

                    QJsonObject requestObj;
                    requestObj["model"] = "deepseek-v4-flash"; 
                    requestObj["messages"] = messagesArray;

                    QNetworkReply* aiReply = m_networkManager->post(aiReq, QJsonDocument(requestObj).toJson());
                    aiReply->setProperty("requestType", "ocr_ai_analysis");
                } else if (reqType == "ocr_ai_analysis") {
                    QString finalText = QString("============== AI 场景分析 ==============\n%1\n==========================================").arg(content);
                    qDebug().noquote() << "\n" << finalText << "\n";
                    showBubble(content);
                }
            }
        } else {
            qDebug() << "Unexpected JSON format:" << response;
        }
    } else {
        qDebug() << "Network API Error:" << reply->errorString();
        qDebug() << "Error detail:" << reply->readAll();
    }
    
    QString fileName = reply->property("fileName").toString();
    if (!fileName.isEmpty()) {
        QFile::remove(fileName);
    }
    
    reply->deleteLater();
}

void DesktopPet::showLoadingBubble() {
    if (m_activeBubble) {
        m_activeBubble->close();
        m_activeBubble = nullptr;
        m_activeBubbleText = nullptr;
        m_activeBubbleInnerLayout = nullptr;
        m_activeBubbleContent = nullptr;
        m_activeBubbleHasWebView = false;
    }

    QWidget* bubble = new QWidget();
    bubble->setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    bubble->setAttribute(Qt::WA_TranslucentBackground);
    bubble->setAttribute(Qt::WA_DeleteOnClose);

    connect(bubble, &QObject::destroyed, this, [this]() {
        m_activeBubble = nullptr;
        m_activeBubbleText = nullptr;
        m_activeBubbleInnerLayout = nullptr;
        m_activeBubbleContent = nullptr;
        m_activeBubbleHasWebView = false;
    });

    QVBoxLayout* bLayout = new QVBoxLayout(bubble);
    bLayout->setContentsMargins(10, 10, 10, 10);

    QWidget* bgWidget = new QWidget(bubble);
    bgWidget->setStyleSheet("background-color: rgba(255, 240, 245, 240); border-radius: 10px; border: 2px solid #ffb6c1;");
    QVBoxLayout* innerLayout = new QVBoxLayout(bgWidget);

    QTextBrowser* textBrowser = new QTextBrowser(bgWidget);
    textBrowser->setMarkdown(QString());
    textBrowser->setStyleSheet("border: none; background: transparent; color: #5c3a41; font-size: 13px;");
    textBrowser->setMinimumSize(300, 200);
    textBrowser->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    innerLayout->addWidget(textBrowser);

    QPushButton* closeBtn = new QPushButton("关闭", bgWidget);
    closeBtn->setStyleSheet("QPushButton { background-color: #ffb6c1; border-radius: 5px; padding: 5px 15px; font-weight: bold; color: #fff; } "
                            "QPushButton:hover { background-color: #ff91a4; }");
    QObject::connect(closeBtn, &QPushButton::clicked, bubble, &QWidget::close);

    innerLayout->addWidget(closeBtn, 0, Qt::AlignRight);
    bLayout->addWidget(bgWidget);

    bubble->adjustSize();
    QRect petRect = this->geometry();
    int bubbleX = petRect.left() - bubble->width() - 10;
    int bubbleY = petRect.top();
    if (bubbleX < 0) {
        bubbleX = petRect.right() + 10;
    }
    bubble->move(bubbleX, bubbleY);
    bubble->show();

    m_activeBubble = bubble;
    m_activeBubbleText = textBrowser;
    m_activeBubbleInnerLayout = innerLayout;
    m_activeBubbleContent = bgWidget;
    m_activeBubbleHasWebView = false;
}

void DesktopPet::showBubble(const QString& text, const QString& url) {
    // 【核心修复】：如果有网页，而现有的气泡是全透明的加载气泡，必须把它彻底关闭并清理，以便重新创建非分层窗口
    if (!url.isEmpty() && m_activeBubble && !m_activeBubbleHasWebView) {
        m_activeBubble->close(); 
        m_activeBubble = nullptr;
        m_activeBubbleText = nullptr;
        m_activeBubbleInnerLayout = nullptr;
        m_activeBubbleContent = nullptr;
    }

    // 如果气泡不存在（被上面释放了，或者原本就没弹出），则重新创建整个气泡结构
    if (!m_activeBubble || !m_activeBubbleText || !m_activeBubbleInnerLayout || !m_activeBubbleContent) {
        QWidget* bubble = new QWidget();
        
        // 【关键改动】：必须在首次调用 show() 之前，把窗口属性彻底定死！
        if (url.isEmpty()) {
            bubble->setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
            bubble->setAttribute(Qt::WA_TranslucentBackground); // 纯文本无网页：允许全透明
        } else {
            bubble->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
            bubble->setAttribute(Qt::WA_TranslucentBackground, false); // 带网页：必须严格关闭全透明
            bubble->setStyleSheet("background-color: rgb(255, 240, 245);"); // 铺满底色防止穿透
        }
        bubble->setAttribute(Qt::WA_DeleteOnClose);

        connect(bubble, &QObject::destroyed, this, [this]() {
            m_activeBubble = nullptr;
            m_activeBubbleText = nullptr;
            m_activeBubbleInnerLayout = nullptr;
            m_activeBubbleContent = nullptr;
            m_activeBubbleHasWebView = false;
        });

        QVBoxLayout* bLayout = new QVBoxLayout(bubble);
        bLayout->setContentsMargins(10, 10, 10, 10);

        QWidget* bgWidget = new QWidget(bubble);
        bgWidget->setStyleSheet("background-color: rgba(255, 240, 245, 240); border-radius: 10px; border: 2px solid #ffb6c1;");
        QVBoxLayout* innerLayout = new QVBoxLayout(bgWidget);

        QTextBrowser* textBrowser = new QTextBrowser(bgWidget);
        textBrowser->setStyleSheet("border: none; background: transparent; color: #5c3a41; font-size: 13px;");
        textBrowser->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

        innerLayout->addWidget(textBrowser);

        QPushButton* closeBtn = new QPushButton("关闭", bgWidget);
        closeBtn->setStyleSheet("QPushButton { background-color: #ffb6c1; border-radius: 5px; padding: 5px 15px; font-weight: bold; color: #fff; } "
                                "QPushButton:hover { background-color: #ff91a4; }");
        QObject::connect(closeBtn, &QPushButton::clicked, bubble, &QWidget::close);

        innerLayout->addWidget(closeBtn, 0, Qt::AlignRight);
        bLayout->addWidget(bgWidget);

        m_activeBubble = bubble;
        m_activeBubbleText = textBrowser;
        m_activeBubbleInnerLayout = innerLayout;
        m_activeBubbleContent = bgWidget;
        m_activeBubbleHasWebView = false;
    }

    // 此时 m_activeBubble 已经是处于正确兼容模式的窗口了，开始填入内容
    m_activeBubbleText->setMarkdown(text);
    m_activeBubbleText->setMinimumSize(300, url.isEmpty() ? 200 : 120);

    // 动态挂载浏览器组件
    if (!url.isEmpty() && !m_activeBubbleHasWebView) {
        NativeWebView* webView = new NativeWebView(url, m_activeBubbleContent);
        webView->setMinimumSize(420, 320);
        webView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_activeBubbleInnerLayout->insertWidget(1, webView); // 塞到文本和关闭按钮中间

        m_activeBubbleHasWebView = true;
    }

    // 调整尺寸并显示
    m_activeBubble->adjustSize();
    QRect petRect = this->geometry();
    int bubbleX = petRect.left() - m_activeBubble->width() - 10;
    int bubbleY = petRect.top();
    if (bubbleX < 0) {
        bubbleX = petRect.right() + 10;
    }
    m_activeBubble->move(bubbleX, bubbleY);
    m_activeBubble->show(); // 此时触发 NativeWebView 的 showEvent，由于父窗口非分层，HWND 将完美渲染
}
