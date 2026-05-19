#include "chatpanel.h"
#include "chatbubble.h"
#include "aiclient.h"
#include "contextbuilder.h"
#include "ChatModel.h"
#include <QScrollBar>
#include <QShortcut>

ChatPanel::ChatPanel(AiClient *aiClient,QWidget *parent)
    :QWidget(parent),m_aiClient(aiClient),m_isWaiting(false)
{
    m_polishBtn    = new QPushButton(QStringLiteral("润色"),    this);
    m_expandBtn    = new QPushButton(QStringLiteral("扩写"),    this);
    m_translateBtn = new QPushButton(QStringLiteral("翻译英文"), this);
    m_summarizeBtn = new QPushButton(QStringLiteral("总结"),    this);

    QHBoxLayout *quickBar = new QHBoxLayout;
    quickBar->setContentsMargins(8,4,8,0);
    quickBar->addWidget(m_polishBtn);
    quickBar->addWidget(m_expandBtn);
    quickBar->addWidget(m_translateBtn);
    quickBar->addWidget(m_summarizeBtn);
    quickBar->addStretch();

    connect(m_polishBtn,    &QPushButton::clicked, this, &ChatPanel::onPolishClicked);
    connect(m_expandBtn,    &QPushButton::clicked, this, &ChatPanel::onExpandClicked);
    connect(m_translateBtn, &QPushButton::clicked, this, &ChatPanel::onTranslateClicked);
    connect(m_summarizeBtn, &QPushButton::clicked, this, &ChatPanel::onSummarizeClicked);

    m_bubbleContainer = new QWidget;
    m_bubbleLayout = new QVBoxLayout(m_bubbleContainer);
    m_bubbleLayout->setContentsMargins(0,0,0,0);
    m_bubbleLayout->addStretch();

    m_scrollArea = new QScrollArea;
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setWidget(m_bubbleContainer);
    m_scrollArea->setStyleSheet(QStringLiteral(
        "QScrollArea{ border: none; background: #FFFFFF; }"
    ));

    m_inputEdit = new QTextEdit;
    m_inputEdit->setPlaceholderText(QStringLiteral("输入消息... (Ctrl+Enter 发送)"));
    m_inputEdit->setMaximumHeight(80);
    m_inputEdit->setStyleSheet(QStringLiteral(
        "QTextEdit {"
        "  border: 1px solid #D0D0D0;"
        "  border-radius: 8px;"
        "  padding: 6px 10px;"
        "  font-size: 13px;"
        "}"
    ));

    m_sendBtn = new QPushButton(QStringLiteral("发送"));
    m_sendBtn->setFixedHeight(36);
    m_sendBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background-color: #0078D4;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 6px;"
        "  padding: 0 20px;"
        "  font-size: 13px;"
        "}"
        "QPushButton:hover  { background-color: #106EBE; }"
        "QPushButton:pressed{ background-color: #005A9E; }"
        ));

    QHBoxLayout *inputRow = new QHBoxLayout;
    inputRow->setContentsMargins(8,0,8,6);
    inputRow->addWidget(m_inputEdit,1);
    inputRow->addWidget(m_sendBtn);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addLayout(quickBar);
    mainLayout->addWidget(m_scrollArea, 1);  // stretch=1：占主空间
    mainLayout->addLayout(inputRow);

    connect(m_sendBtn,&QPushButton::clicked,this,&ChatPanel::onSendClicked);
    QShortcut *shortcut = new QShortcut(QKeySequence("Ctrl+Return"), this);
    connect(shortcut, &QShortcut::activated,this, &ChatPanel::onSendClicked);

    connect(m_aiClient, &AiClient::streamChunk,this, &ChatPanel::onStreamChunk);
    connect(m_aiClient, &AiClient::responseDone,this, &ChatPanel::onResponseDone);
    connect(m_aiClient, &AiClient::errorOccurred,this, &ChatPanel::onError);

}

void  ChatPanel::setDocumentContent(const QString &content)
{
    m_documentContent = content;
}

void ChatPanel::onSendClicked()
{
    if(m_isWaiting)return;

    QString text = m_inputEdit->toPlainText().trimmed();
    if(text.isEmpty())return;

    ChatBubble *userBubble = createBubble(ChatBubble::User);
    userBubble->setText(text);

    m_chatModel.addMessage(ChatModel::User,text);

    createBubble(ChatBubble::Assistant);
    currentAiBubble()->setState(ChatBubble::Waiting);

    m_inputEdit->clear();
    setInputEnabled(false);

    QJsonArray fullMessages;

    QJsonObject sysObj;
    sysObj["role"]    = QStringLiteral("system");
    sysObj["content"] = ContextBuilder::systemPrompt();
    fullMessages.append(sysObj);

    const auto &history = m_chatModel.messages();
    for(int i = 0;i < history.size()-1;++i){
        QJsonObject histObj;
        histObj["role"] = (history[i].role == ChatModel::User)
                              ? QStringLiteral("user")
                              : QStringLiteral("assistant");
        histObj["content"] = history[i].content;
        fullMessages.append(histObj);
    }

    QJsonObject curObj;
    curObj["role"] = QStringLiteral("user");
    if (m_documentContent.isEmpty()) {
        curObj["content"] = text;
    } else {
        curObj["content"] = QStringLiteral(
            "我正在编辑以下 Markdown 文档:\n\n"
            "```markdown\n%1\n```\n\n"
            "%2"
        ).arg(m_documentContent, text);
    }
    fullMessages.append(curObj);

    m_aiClient->sendMessage(fullMessages);
}

void ChatPanel::onStreamChunk(const QString &chunk)
{
    ChatBubble *bubble = currentAiBubble();
    if(!bubble)return;

    if(bubble->state() == ChatBubble::Waiting)
    {
        bubble->setState(ChatBubble::Streaming);
        bubble->setText(QString());
    }

    bubble->appendText(chunk);
    scrollToBottom();
}

void ChatPanel::onResponseDone()
{
    ChatBubble *bubble = currentAiBubble();
    if(bubble)
    {
        bubble->setState(ChatBubble::Done);
        m_chatModel.addMessage(ChatModel::Assistant,bubble->text());

    }
    setInputEnabled(true);
    scrollToBottom();
}

void ChatPanel::onError(const QString &error)
{
    ChatBubble *bubble = currentAiBubble();
    if(bubble)
    {
        bubble->setText(QStringLiteral("错误: ") + error);
        bubble->setState(ChatBubble::Done);
    }
    setInputEnabled(true);
    scrollToBottom();
}

void ChatPanel::onPolishClicked()
{
    fillPromptAndFocus(ContextBuilder::polishPrompt());
}

void ChatPanel::onExpandClicked()
{
    fillPromptAndFocus(ContextBuilder::expandPrompt());
}

void ChatPanel::onTranslateClicked()
{
    fillPromptAndFocus(ContextBuilder::translatePrompt());
}

void ChatPanel::onSummarizeClicked()
{
    fillPromptAndFocus(ContextBuilder::summarizePrompt());
}

void ChatPanel::fillPromptAndFocus(const QString &prefix)
{
    m_inputEdit->setPlainText(prefix);
    m_inputEdit->moveCursor(QTextCursor::End);
    m_inputEdit->setFocus();
}

ChatBubble *ChatPanel::createBubble(ChatBubble::Role role)
{
    ChatBubble *bubble = new ChatBubble(role,m_bubbleContainer);
    int stretchIndex = m_bubbleLayout->count()-1;
    m_bubbleLayout->insertWidget(stretchIndex,bubble);
    return bubble;
}

ChatBubble *ChatPanel::currentAiBubble() const
{
    for(int i = m_bubbleLayout->count()-1;i>=0;--i)
    {
        QLayoutItem *item = m_bubbleLayout->itemAt(i);
        if(!item || !item->widget())continue;

        ChatBubble *bubble = qobject_cast<ChatBubble *>(item->widget());
        if(bubble && bubble->role() == ChatBubble::Assistant)
        {
            return bubble;
        }
    }
    return nullptr;
}

void ChatPanel::scrollToBottom()
{
    QScrollBar *vbar = m_scrollArea->verticalScrollBar();
    vbar->setValue(vbar->maximum());
}

void ChatPanel::setInputEnabled(bool enabled)
{
    m_isWaiting = !enabled;
    m_inputEdit->setEnabled(enabled);
    m_sendBtn->setEnabled(enabled);
}
