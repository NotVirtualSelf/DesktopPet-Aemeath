#ifndef AI_CHAT_DIALOG_H
#define AI_CHAT_DIALOG_H

#include <QDialog>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class AIChatDialog : public QDialog {
    Q_OBJECT

public:
    explicit AIChatDialog(QWidget *parent = nullptr);
    ~AIChatDialog();

private slots:
    void onSendClicked();
    void aiRespond(const QString& userText);
    void onNetworkReply(QNetworkReply* reply);

private:
    void addMessage(const QString& text, bool isUser);
    void setupUi();

    QScrollArea* m_scrollArea;
    QWidget* m_scrollWidget;
    QVBoxLayout* m_messagesLayout;
    
    QLineEdit* m_inputLineEdit;
    QPushButton* m_sendButton;
    
    QNetworkAccessManager* m_networkManager;
};

#endif // AI_CHAT_DIALOG_H