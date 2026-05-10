#include "ai_chat_dialog.h"
#include <QScrollBar>
#include <QTimer>
#include <QApplication>

AIChatDialog::AIChatDialog(QWidget *parent)
    : QDialog(parent) {
    setupUi();
    resize(400, 550);
    setWindowTitle("AI 助手");
    setStyleSheet("QDialog { background-color: #f5f5f5; }");
    
    connect(m_sendButton, &QPushButton::clicked, this, &AIChatDialog::onSendClicked);
    connect(m_inputLineEdit, &QLineEdit::returnPressed, this, &AIChatDialog::onSendClicked);
    
    // Initial greeting
    addMessage("你好！我是爱弥斯，有什么我可以帮你的吗？", false);
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
        "QLineEdit { border: 1px solid #ddd; border-radius: 17px; padding: 0 15px; background: white; font-size: 14px; }"
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

    // Simulate AI typing delay
    QTimer::singleShot(1000, this, [this, text]() {
        aiRespond(text);
    });
}

void AIChatDialog::aiRespond(const QString& userText) {
    // Placeholder AI integration. You can connect to your ChatGPT/LLM API here.
    QString reply = QString("我已经收到你的消息: \"%1\"。\n这里是AI接口的占位符！后续可以接入大模型API。").arg(userText);
    addMessage(reply, false);
}