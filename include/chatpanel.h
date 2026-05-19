#ifndef CHATPANEL_H
#define CHATPANEL_H

#include <QWidget>
#include <QScrollArea>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include "ChatModel.h"
#include "chatbubble.h"

class AiClient;

class ChatPanel : public QWidget
{
    Q_OBJECT
public:
    explicit ChatPanel(AiClient *aiClient,QWidget *parent = nullptr);
    void setDocumentContent(const QString &content);

private slots:
    void onSendClicked();
    void onStreamChunk(const QString &chunk);
    void onResponseDone();
    void onError(const QString &error);

    void onPolishClicked();
    void onExpandClicked();
    void onTranslateClicked();
    void onSummarizeClicked();

private:
    ChatBubble *createBubble(ChatBubble::Role role);
    ChatBubble *currentAiBubble() const;
    void  scrollToBottom();
    void  setInputEnabled(bool enabled);
    void  fillPromptAndFocus(const QString &prefix);

    // UI 组件
    QScrollArea *m_scrollArea;
    QWidget     *m_bubbleContainer;   // QScrollArea 的内部容器
    QVBoxLayout *m_bubbleLayout;      // 气泡列表的垂直布局

    QTextEdit   *m_inputEdit;
    QPushButton *m_sendBtn;

    QPushButton *m_polishBtn;
    QPushButton *m_expandBtn;
    QPushButton *m_translateBtn;
    QPushButton *m_summarizeBtn;

    // 数据
    AiClient *m_aiClient;    // 不持有所有权，外部传入
    ChatModel m_chatModel;   // 对话历史
    QString   m_documentContent;
    bool      m_isWaiting;   // 等待 AI 回复时锁定输入
};

#endif // CHATPANEL_H
