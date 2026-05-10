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

class AIChatDialog : public QDialog {
    Q_OBJECT

public:
    explicit AIChatDialog(QWidget *parent = nullptr);
    ~AIChatDialog();

private slots:
    void onSendClicked();
    void aiRespond(const QString& userText);

private:
    void addMessage(const QString& text, bool isUser);
    void setupUi();

    QScrollArea* m_scrollArea;
    QWidget* m_scrollWidget;
    QVBoxLayout* m_messagesLayout;
    
    QLineEdit* m_inputLineEdit;
    QPushButton* m_sendButton;
};

#endif // AI_CHAT_DIALOG_H