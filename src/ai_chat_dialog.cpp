#include "ai_chat_dialog.h"
#include <QScrollBar>
#include <QTimer>
#include <QApplication>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

const char* chat_apiKey = "sk-3689655a2d864015924989050c091e31";

AIChatDialog::AIChatDialog(QWidget *parent)
    : QDialog(parent) {
    setupUi();
    resize(400, 550);
    setWindowTitle("AI 聊天");
    setStyleSheet("QDialog { background-color: #f5f5f5; }");
    
    m_networkManager = new QNetworkAccessManager(this);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &AIChatDialog::onNetworkReply);
    
    connect(m_sendButton, &QPushButton::clicked, this, &AIChatDialog::onSendClicked);
    connect(m_inputLineEdit, &QLineEdit::returnPressed, this, &AIChatDialog::onSendClicked);
    
    // Initial greeting
    addMessage("你好！有什么我可以帮你的吗？", false);
}

AIChatDialog::~AIChatDialog() {}

void AIChatDialog::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setStyleSheet("QScrollArea { border: none; background-color: transparent; }");

    m_scrollWidget = new QWidget(m_scrollArea);
    m_scrollWidget->setStyleSheet("QWidget { background-color: transparent; }");
    m_messagesLayout = new QVBoxLayout(m_scrollWidget);
    m_messagesLayout->setAlignment(Qt::AlignTop);
    m_messagesLayout->setContentsMargins(0, 0, 0, 0);
    m_messagesLayout->setSpacing(8);

    m_scrollArea->setWidget(m_scrollWidget);

    QHBoxLayout* inputLayout = new QHBoxLayout();
    
    m_inputLineEdit = new QLineEdit(this);
    m_inputLineEdit->setPlaceholderText("你想说点什么...");
    m_inputLineEdit->setMinimumHeight(35);
    m_inputLineEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #ddd; border-radius: 17px; padding: 0 15px; background: white; color: #333333; font-size: 14px; }"
    );
    
    m_sendButton = new QPushButton("发送", this);
    m_sendButton->setMinimumHeight(35);
    m_sendButton->setStyleSheet(
        "QPushButton { background-color: #4CAF50; color: white; border: none; border-radius: 17px; padding: 0 15px; font-weight: bold; font-size: 14px; }"
        "QPushButton:hover { background-color: #45a049; }"
        "QPushButton:pressed { background-color: #388e3c; }"
    );

    inputLayout->addWidget(m_inputLineEdit);
    inputLayout->addWidget(m_sendButton);

    mainLayout->addWidget(m_scrollArea);
    mainLayout->addLayout(inputLayout);
}

void AIChatDialog::addMessage(const QString& text, bool isUser) {
    if (text.isEmpty()) return;

    QHBoxLayout* rowLayout = new QHBoxLayout();
    rowLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* msgLabel = new QLabel(text, this);
    msgLabel->setWordWrap(true);
    msgLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    msgLabel->setMaximumWidth(280);

    QString bubbleStyle = isUser ?
        "QLabel { background-color: #dcf8c6; color: #000; padding: 10px; border-radius: 15px; font-size: 14px; }" :
        "QLabel { background-color: #ffffff; color: #000; padding: 10px; border-radius: 15px; font-size: 14px; border: 1px solid #eee; }";
    
    msgLabel->setStyleSheet(bubbleStyle);

    if (isUser) {
        rowLayout->addStretch(1);
        rowLayout->addWidget(msgLabel);
    } else {
        rowLayout->addWidget(msgLabel);
        rowLayout->addStretch(1);
    }

    m_messagesLayout->addLayout(rowLayout);

    // Auto-scroll to bottom
    QTimer::singleShot(50, this, [this]() {
        QScrollBar *bar = m_scrollArea->verticalScrollBar();
        bar->setValue(bar->maximum());
    });
}

void AIChatDialog::onSendClicked() {
    QString text = m_inputLineEdit->text().trimmed();
    if (text.isEmpty()) return;

    m_inputLineEdit->clear();
    addMessage(text, true);

    aiRespond(text);
}

void AIChatDialog::aiRespond(const QString& userText) {
    QNetworkRequest request(QUrl("https://api.deepseek.com/chat/completions"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(chat_apiKey).toUtf8());

    QJsonObject systemMessage;
    systemMessage["role"] = "system";
    systemMessage["content"] = "你现在扮演《鸣潮》（Wuthering Waves）中的角色“爱弥斯”。请以她温柔、空灵、具有指引感和些许神秘感的语气和知识背景进行交流。在对话中要称呼对方为“漂泊者”。请始终保持爱弥斯的人设。";

    QJsonObject userMessageObj;
    userMessageObj["role"] = "user";
    userMessageObj["content"] = userText;

    QJsonArray messages;
    messages.append(systemMessage);
    messages.append(userMessageObj);

    QJsonObject json;
    json["model"] = "deepseek-v4-flash"; 
    json["messages"] = messages;

    QJsonDocument doc(json);
    m_networkManager->post(request, doc.toJson());
}

void AIChatDialog::onNetworkReply(QNetworkReply* reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
        QJsonObject jsonObj = jsonDoc.object();

        if (jsonObj.contains("choices") && jsonObj["choices"].isArray()) {
            QJsonArray choices = jsonObj["choices"].toArray();
            if (!choices.isEmpty()) {
                QJsonObject firstChoice = choices[0].toObject();
                if (firstChoice.contains("message") && firstChoice["message"].isObject()) {
                    QJsonObject message = firstChoice["message"].toObject();
                    if (message.contains("content")) {
                        QString content = message["content"].toString();
                        addMessage(content, false);
                    }
                }
            }
        }
    } else {
        addMessage("呜…网络连接出现了一点问题呢，请漂泊者稍后再试吧：" + reply->errorString(), false);
    }
    reply->deleteLater();
}