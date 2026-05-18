# 第三批：ChatPanel + 改 MainWindow + 改 CMakeLists

**最后一批。写完这个，编译，放入 API Key，跑起来就能和 AI 对话。**

---

## 一、ChatPanel — AI 面板 UI 总指挥

### 它在屏幕上长什么样

```
┌──────────────────────────────────────────────┐
│ [润色] [扩写] [翻译英文] [总结]                │  ← 快捷按钮栏 (QHBoxLayout)
├──────────────────────────────────────────────┤
│ ┌──────────────────────────────────┐         │
│ │                    用户：帮我看看 │  蓝色   │  ← 气泡列表
│ └──────────────────────────────────┘         │     (QScrollArea
│ ┌──────────────────────────────────┐         │      + 内部 VBoxLayout
│ │ AI：好的，这段可以改成...         │  灰色   │      每个气泡是 ChatBubble)
│ └──────────────────────────────────┘         │
│                                              │
├──────────────────────────────────────────────┤
│ 输入消息... (Ctrl+Enter 发送)       [发送]   │  ← 输入行 (QHBoxLayout)
└──────────────────────────────────────────────┘
```

总布局：顶层 `QVBoxLayout`，从上到下装三样东西——快捷按钮栏、QScrollArea（可滚动的气泡列表）、输入行。

### chatpanel.h — 完整代码

```cpp
#ifndef CHATPANEL_H
#define CHATPANEL_H

#include <QWidget>
#include <QScrollArea>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include "chatmodel.h"
#include "chatbubble.h"     // 需要 ChatBubble::Role 枚举

class AiClient;  // 仅用指针，前置声明足够

class ChatPanel : public QWidget
{
    Q_OBJECT
public:
    explicit ChatPanel(AiClient *aiClient, QWidget *parent = nullptr);

    // 外部（MainWindow）调用：更新编辑器全文，用于 AI 上下文
    void setDocumentContent(const QString &content);

private slots:
    void onSendClicked();
    void onStreamChunk(const QString &chunk);
    void onResponseDone();
    void onError(const QString &error);

    // 快捷按钮
    void onPolishClicked();
    void onExpandClicked();
    void onTranslateClicked();
    void onSummarizeClicked();

private:
    ChatBubble *createBubble(ChatBubble::Role role);
    ChatBubble *currentAiBubble() const;
    void        scrollToBottom();
    void        setInputEnabled(bool enabled);
    void        fillPromptAndFocus(const QString &prefix);

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
```

**逐行解释：**

- **`AiClient *m_aiClient`（指针，不持有所有权）**：ChatPanel 不 new AiClient，不 delete AiClient。谁创建 ChatPanel（MainWindow）谁负责创建和管理 AiClient 的生命周期。这叫"依赖注入"——ChatPanel 只需要"能发消息的东西"，不关心这东西是 DeepSeek 还是别的。

- **`ChatModel m_chatModel`（值对象，不是指针）**：ChatModel 是 ChatPanel 的一部分，ChatPanel 创建它、销毁它。因为 ChatModel 只是数据容器，不需要多态。

- **`m_isWaiting`**：防连发锁。用户按发送后设为 true，AI 回复完（responseDone/errorOccurred）设为 false。等待期间输入框和发送按钮都禁用。

- **前置声明 `class AiClient;` vs include**：`AiClient*` 只需要前置声明。但 `ChatBubble::Role` 是枚举，编译器需要知道具体值，所以必须 `#include "chatbubble.h"`。

---

### chatpanel.cpp — 完整代码（上）：构造

```cpp
#include "chatpanel.h"
#include "chatbubble.h"
#include "aiclient.h"
#include "contextbuilder.h"

#include <QScrollBar>
#include <QShortcut>

ChatPanel::ChatPanel(AiClient *aiClient, QWidget *parent)
    : QWidget(parent)
    , m_aiClient(aiClient)
    , m_isWaiting(false)
{
    // ──── 1. 快捷按钮栏 ────
    m_polishBtn    = new QPushButton(QStringLiteral("润色"),    this);
    m_expandBtn    = new QPushButton(QStringLiteral("扩写"),    this);
    m_translateBtn = new QPushButton(QStringLiteral("翻译英文"), this);
    m_summarizeBtn = new QPushButton(QStringLiteral("总结"),    this);

    QHBoxLayout *quickBar = new QHBoxLayout;
    quickBar->setContentsMargins(8, 4, 8, 0);
    quickBar->addWidget(m_polishBtn);
    quickBar->addWidget(m_expandBtn);
    quickBar->addWidget(m_translateBtn);
    quickBar->addWidget(m_summarizeBtn);
    quickBar->addStretch();   // 弹簧：按钮靠左，右边留白

    connect(m_polishBtn,    &QPushButton::clicked, this, &ChatPanel::onPolishClicked);
    connect(m_expandBtn,    &QPushButton::clicked, this, &ChatPanel::onExpandClicked);
    connect(m_translateBtn, &QPushButton::clicked, this, &ChatPanel::onTranslateClicked);
    connect(m_summarizeBtn, &QPushButton::clicked, this, &ChatPanel::onSummarizeClicked);

    // ──── 2. 气泡列表（QScrollArea）────
    m_bubbleContainer = new QWidget;
    m_bubbleLayout    = new QVBoxLayout(m_bubbleContainer);
    m_bubbleLayout->setContentsMargins(0, 0, 0, 0);
    m_bubbleLayout->addStretch();  // 底部弹簧：气泡从顶部开始，下方留白

    m_scrollArea = new QScrollArea;
    m_scrollArea->setWidgetResizable(true);    // ← 关键
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setWidget(m_bubbleContainer);
    m_scrollArea->setStyleSheet(QStringLiteral(
        "QScrollArea { border: none; background: #FFFFFF; }"
    ));

    // ──── 3. 输入行 ────
    m_inputEdit = new QTextEdit;
    m_inputEdit->setPlaceholderText(
        QStringLiteral("输入消息... (Ctrl+Enter 发送)"));
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
    inputRow->setContentsMargins(8, 0, 8, 6);
    inputRow->addWidget(m_inputEdit, 1);  // stretch=1：输入框占满剩余宽度
    inputRow->addWidget(m_sendBtn);

    // ──── 4. 拼装总布局 ────
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addLayout(quickBar);
    mainLayout->addWidget(m_scrollArea, 1);  // stretch=1：占主空间
    mainLayout->addLayout(inputRow);

    // ──── 5. 信号连接 ────
    connect(m_sendBtn, &QPushButton::clicked,
            this, &ChatPanel::onSendClicked);

    // Ctrl+Enter 快捷键
    QShortcut *shortcut = new QShortcut(QKeySequence("Ctrl+Return"), this);
    connect(shortcut, &QShortcut::activated,
            this, &ChatPanel::onSendClicked);

    // AI 客户端信号
    connect(m_aiClient, &AiClient::streamChunk,
            this, &ChatPanel::onStreamChunk);
    connect(m_aiClient, &AiClient::responseDone,
            this, &ChatPanel::onResponseDone);
    connect(m_aiClient, &AiClient::errorOccurred,
            this, &ChatPanel::onError);
}
```

**逐行解释：**

- **`m_bubbleLayout->addStretch()`**（初始弹簧）：这是整个气泡列表布局最巧妙的一行。初始时 m_bubbleLayout 里只有一个 stretch（弹簧），它吃掉所有空白空间，所以气泡列表虽然是空的，但气泡区不会塌陷。新气泡用 `insertWidget(stretchIndex, bubble)` 插入到弹簧**之前**，弹簧始终在底部。当气泡足够多时，弹簧被推到可视区外，QScrollArea 的滚动条接管。

- **`setWidgetResizable(true)`**：QScrollArea 默认不会自动调整内部 widget 大小。设 true 后，m_bubbleContainer 大小变化时 QScrollArea 会自动更新滚动范围。不设的话气泡超过可视区时不会出现滚动条，后面的气泡看不见。

- **`addWidget(m_inputEdit, 1)`**：stretch factor = 1，输入框占满输入行的剩余宽度。发送按钮宽度固定（约 60px），输入框拿到剩下所有空间。

- **`QShortcut("Ctrl+Return")`**：Qt 的跨平台快捷键。`Ctrl+Return` 在 Windows/Linux 生效，macOS 上 Qt 自动映射成 `Cmd+Return`。

- **信号连接顺序**：三个 AI 信号直接连到 ChatPanel 的对应 slot。ChatPanel 不需要知道 AiClient 的底层实现。

---

### chatpanel.cpp — 完整代码（中）：发送流程

```cpp
// ═══════════════ 公开方法 ═══════════════

void ChatPanel::setDocumentContent(const QString &content)
{
    m_documentContent = content;
}

// ═══════════════ 发送 ═══════════════

void ChatPanel::onSendClicked()
{
    if (m_isWaiting) return;   // 正在等 AI 回复，不允许连发

    QString text = m_inputEdit->toPlainText().trimmed();
    if (text.isEmpty()) return;

    // 1. 创建用户气泡
    ChatBubble *userBubble = createBubble(ChatBubble::User);
    userBubble->setText(text);

    // 2. 存历史
    m_chatModel.addMessage(ChatModel::User, text);

    // 3. 创建 AI 气泡（Waiting 状态，显示 "..."）
    createBubble(ChatBubble::Assistant);
    currentAiBubble()->setState(ChatBubble::Waiting);

    // 4. 清空输入框 + 锁定
    m_inputEdit->clear();
    setInputEnabled(false);

    // 5. 构造完整的 messages 数组
    QJsonArray fullMessages;

    // 5a: system prompt
    QJsonObject sysObj;
    sysObj["role"]    = QStringLiteral("system");
    sysObj["content"] = ContextBuilder::systemPrompt();
    fullMessages.append(sysObj);

    // 5b: 历史消息（不含最后一条，因为马上要加带文档上下文的版本）
    const auto &history = m_chatModel.messages();
    for (int i = 0; i < history.size() - 1; ++i) {
        QJsonObject histObj;
        histObj["role"] = (history[i].role == ChatModel::User)
                          ? QStringLiteral("user")
                          : QStringLiteral("assistant");
        histObj["content"] = history[i].content;
        fullMessages.append(histObj);
    }

    // 5c: 当前消息（带文档全文上下文）
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

    // 6. 发送
    m_aiClient->sendMessage(fullMessages);
}
```

**逐行解释——多轮对话的消息数组是怎么拼的：**

假设现在是第 3 轮对话：
- `history[0]`: User "帮我写标题"
- `history[1]`: Assistant "好的..."
- `history[2]`: User "再润色一下"（刚加的）

构建 fullMessages：
1. system 消息
2. history[0] → user
3. history[1] → assistant
4. history[2] → **跳过**（`for (i = 0; i < history.size()-1; ++i)` 不包含最后一条）
5. 当前消息（带文档上下文）→ user

**为什么 history[2] 要跳过、然后重新构造？** 因为最后一条需要带上编辑器全文上下文（用 `\`\`\`markdown` 包裹）。ChatModel 里存的是纯用户输入文本（没有文档上下文），所以跳过它，用带上下文的版本替代。

---

### chatpanel.cpp — 完整代码（下）：流式接收 + 辅助方法

```cpp
// ═══════════════ 流式接收 ═══════════════

void ChatPanel::onStreamChunk(const QString &chunk)
{
    ChatBubble *bubble = currentAiBubble();
    if (!bubble) return;

    // 第一个 chunk 到达时：从 Waiting 切到 Streaming
    if (bubble->state() == ChatBubble::Waiting) {
        bubble->setState(ChatBubble::Streaming);
        bubble->setText(QString());  // 清掉 "..."
    }

    bubble->appendText(chunk);
    scrollToBottom();
}

void ChatPanel::onResponseDone()
{
    ChatBubble *bubble = currentAiBubble();
    if (bubble) {
        bubble->setState(ChatBubble::Done);
        m_chatModel.addMessage(ChatModel::Assistant, bubble->text());
    }
    setInputEnabled(true);
    scrollToBottom();
}

void ChatPanel::onError(const QString &error)
{
    ChatBubble *bubble = currentAiBubble();
    if (bubble) {
        bubble->setText(QStringLiteral("错误: ") + error);
        bubble->setState(ChatBubble::Done);
    }
    setInputEnabled(true);
    scrollToBottom();
}

// ═══════════════ 快捷按钮 ═══════════════

void ChatPanel::onPolishClicked()
    { fillPromptAndFocus(ContextBuilder::polishPrompt()); }

void ChatPanel::onExpandClicked()
    { fillPromptAndFocus(ContextBuilder::expandPrompt()); }

void ChatPanel::onTranslateClicked()
    { fillPromptAndFocus(ContextBuilder::translatePrompt()); }

void ChatPanel::onSummarizeClicked()
    { fillPromptAndFocus(ContextBuilder::summarizePrompt()); }

void ChatPanel::fillPromptAndFocus(const QString &prefix)
{
    m_inputEdit->setPlainText(prefix);
    m_inputEdit->moveCursor(QTextCursor::End);
    m_inputEdit->setFocus();
}

// ═══════════════ 内部辅助 ═══════════════

ChatBubble *ChatPanel::createBubble(ChatBubble::Role role)
{
    ChatBubble *bubble = new ChatBubble(role, m_bubbleContainer);

    // stretch 始终在 m_bubbleLayout 最后一项
    // 把新气泡插入到 stretch 之前
    int stretchIndex = m_bubbleLayout->count() - 1;
    m_bubbleLayout->insertWidget(stretchIndex, bubble);

    return bubble;
}

ChatBubble *ChatPanel::currentAiBubble() const
{
    // 从后往前找第一个 Assistant 气泡
    for (int i = m_bubbleLayout->count() - 1; i >= 0; --i) {
        QLayoutItem *item = m_bubbleLayout->itemAt(i);
        if (!item || !item->widget()) continue;

        ChatBubble *bubble = qobject_cast<ChatBubble *>(item->widget());
        if (bubble && bubble->role() == ChatBubble::Assistant) {
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
```

**逐行解释：**

- **`onStreamChunk` 里状态切换**：第一个 chunk 到达时气泡还在 Waiting 状态（显示 "..."）。切到 Streaming + 清空 label → 开始追字。后续 chunk 直接 appendText。

- **`onResponseDone` 里 `m_chatModel.addMessage(Assistant, bubble->text())`**：AI 回复完成后把全文存入 ChatModel。下一次用户发消息时，这段回复会作为历史被带上——这就是 AI "记住"之前对话的方式。

- **`scrollToBottom()`**：每次收到新字都滚到底，确保最新回复始终在视野里。`vbar->setValue(vbar->maximum())` 把滚动条设到最大值（最底端）。

- **`qobject_cast<ChatBubble *>`**：Qt 的安全类型转换。如果 widget 不是 ChatBubble，返回 nullptr。比 C 风格 `(ChatBubble*)` 安全——不会把非 ChatBubble 的 widget 强转出错。

- **`createBubble` 里 `insertWidget(stretchIndex, ...)`**：stretchIndex = 最后一个 layout item 的索引（弹簧）。insertWidget 把气泡塞到弹簧前面，弹簧保持压底。如果用 addWidget，弹簧会跑到新气泡上面去。

- **`fillPromptAndFocus`**：快捷按钮填入预设文本，光标移到末尾，输入框获得焦点——用户可以直接粘贴内容或打字，不需要再点输入框。

---

## 二、修改 MainWindow

### 改 mainwindow.h

在文件顶部（前置声明区域，第 17 行 class VirtualizedEditor 之后）加：

```cpp
class ChatPanel;
class DeepSeekClient;
```

在 `private slots:` 区域（第 66 行 `showFindDialog();` 之后）加：

```cpp
    void toggleAIPanel();
```

在 `private:` 函数声明区域（第 68 行 `setupUI();` 之后）加：

```cpp
    void setupAIPanel();
```

在 UI 组件区域（第 93 行 `QSplitter *m_splitter;` 之前）加：

```cpp
    QSplitter *m_verticalSplitter;  // 纵向：上方3栏 + 下方AI面板
```

在差分渲染区域（第 163 行 `QTimer *m_updateTimer;` 之后、`};` 之前）加：

```cpp
    // AI 助手
    ChatPanel      *m_chatPanel = nullptr;
    DeepSeekClient *m_aiClient  = nullptr;
    QAction        *m_toggleAIAction = nullptr;
    bool            m_aiPanelVisible = true;
```

---

### 改 mainwindow.cpp

**第一步**：在顶部 include 区域（第 7 行 `#include "fileencodingdetector.h"` 之后）加：

```cpp
#include "chatpanel.h"
#include "deepseekclient.h"
```

**第二步**：修改 `setupUI()` 函数的结尾部分。找到这几行（大约在第 217-225 行）：

```cpp
    // 设置分割器比例
    m_splitter->setSizes({100, 500, 500});
    m_splitter->setHandleWidth(2);

    // 设置中心部件
    setCentralWidget(m_splitter);

    // 初始化行号显示
    updateLineNumbers();
}
```

**替换为：**

```cpp
    // 设置分割器比例
    m_splitter->setSizes({100, 500, 500});
    m_splitter->setHandleWidth(2);

    // ── 创建 AI 面板 ──
    setupAIPanel();

    // ── 纵向分割器：上方 3 栏编辑器 + 下方 AI 面板 ──
    m_verticalSplitter = new QSplitter(Qt::Vertical, this);
    m_verticalSplitter->addWidget(m_splitter);
    m_verticalSplitter->addWidget(m_chatPanel);
    m_verticalSplitter->setStretchFactor(0, 3);  // 编辑区占 3/4
    m_verticalSplitter->setStretchFactor(1, 1);  // AI 面板占 1/4
    m_verticalSplitter->setHandleWidth(2);

    // 设置中心部件
    setCentralWidget(m_verticalSplitter);

    // 初始化行号显示
    updateLineNumbers();
}

void MainWindow::setupAIPanel()
{
    m_aiClient = new DeepSeekClient(this);
    m_chatPanel = new ChatPanel(m_aiClient, this);

    // 用户发消息 → 传给 AI
    connect(m_chatPanel, &ChatPanel::messageReady,
            m_aiClient,  &AiClient::sendMessage);
}

void MainWindow::toggleAIPanel()
{
    m_aiPanelVisible = !m_aiPanelVisible;
    m_chatPanel->setVisible(m_aiPanelVisible);
    m_toggleAIAction->setChecked(m_aiPanelVisible);
}
```

**第三步**：在 View 菜单里加入 AI 开关。找到 `setupMenu()` 里 `m_toggleDarkThemeAction` 那几行（大约第 385-389 行）：

```cpp
    m_toggleDarkThemeAction = new QAction("暗色主题(&D)", this);
    m_toggleDarkThemeAction->setStatusTip("切换亮色/暗色主题");
    m_toggleDarkThemeAction->setCheckable(true);
    m_toggleDarkThemeAction->setChecked(false);
    m_viewMenu->addAction(m_toggleDarkThemeAction);
```

**紧接着在后面加：**

```cpp
    m_toggleAIAction = new QAction("AI 助手(&I)", this);
    m_toggleAIAction->setStatusTip("显示/隐藏 AI 写作助手");
    m_toggleAIAction->setCheckable(true);
    m_toggleAIAction->setChecked(true);
    m_viewMenu->addAction(m_toggleAIAction);
```

**第四步**：在 `setupConnections()` 里连接 AI 开关信号。找到（大约第 550 行）：

```cpp
    connect(m_toggleDarkThemeAction, &QAction::triggered, this, &MainWindow::toggleDarkTheme);
```

**紧接着加：**

```cpp
    connect(m_toggleAIAction,      &QAction::triggered, this, &MainWindow::toggleAIPanel);
```

**第五步**：在 `updatePreview()` 函数**最开头**（第 841 行左右）加：

```cpp
void MainWindow::updatePreview()
{
    // 同步文档内容到 AI 面板上下文
    if (m_chatPanel) {
        m_chatPanel->setDocumentContent(editorFullContent());
    }

    QString markdownText = editorFullContent();
    // ... 后面原有代码不变
```

---

### 关键解释

- **`setStretchFactor(0, 3)` 和 `setStretchFactor(1, 1)`**：纵向 QSplitter 有 2 个子 widget。索引 0（编辑区）占 3 份，索引 1（AI 面板）占 1 份。总共 4 份 = 75% / 25% 比例。用户可以拖分割线手动调整。

- **`toggleAIPanel()`**：`setVisible(false)` 让 ChatPanel 消失，QSplitter 自动把所有空间给上层编辑区。`setVisible(true)` 恢复。用户不想要 AI 时可以收起来。

- **文档上下文同步**：每次预览更新时调用 `m_chatPanel->setDocumentContent(editorFullContent())`，确保 AI 总能看到编辑器最新内容。

---

## 三、修改 CMakeLists.txt

**第一步**：在 `find_package` 里加 `Network`：

```cmake
find_package(Qt6  REQUIRED COMPONENTS
    Core
    Widgets
    PrintSupport
    Network        # ← 新增
)
```

**第二步**：在 `set(SOURCES ...)` 里加 5 个新 .cpp：

```cmake
set(SOURCES
    main.cpp
    mainwindow.cpp
    markdownparser.cpp
    markdownhighlighter.cpp
    editorhighlighter.cpp
    fileencodingdetector.cpp
    appsettings.cpp
    diffrenderer.cpp
    virtualizededitor.cpp
    chatbubble.cpp          # ← 新增
    chatmodel.cpp           # ← 新增
    chatpanel.cpp           # ← 新增
    deepseekclient.cpp      # ← 新增
    contextbuilder.cpp      # ← 新增
    ${RESOURCE_SOURCES}
)
```

**第三步**：在 `set(HEADERS ...)` 里加 6 个新 .h：

```cmake
set(HEADERS
    mainwindow.h
    markdownparser.h
    markdownhighlighter.h
    editorhighlighter.h
    fileencodingdetector.h
    appsettings.h
    diffrenderer.h
    virtualizededitor.h
    chatbubble.h            # ← 新增
    chatmodel.h             # ← 新增
    aiclient.h              # ← 新增
    chatpanel.h             # ← 新增
    deepseekclient.h        # ← 新增
    contextbuilder.h        # ← 新增
)
```

**第四步**：在 `target_link_libraries` 里加 `Qt6::Network`：

```cmake
target_link_libraries(MarkdownEditor
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::PrintSupport
        Qt6::Network        # ← 新增
)
```

**为什么要加 Network 模块？** DeepSeekClient 用了 `QNetworkAccessManager`、`QNetworkReply`、`QNetworkRequest`，这些都在 Qt Network 模块里。不链接的话会报 `undefined reference`。

---

## 四、编译和运行

### 1. 创建 API Key 文件

在可执行文件目录下（Qt Creator 通常是 `build/Desktop_Qt_xxx/debug/`）创建 `deepseek_key.conf`，内容是：

```
sk-your-deepseek-api-key
```

一行，纯文本，不要多余空格和换行。

### 2. 编译

```bash
cd build
cmake ..
cmake --build .
```

或用 Qt Creator 直接编译。

### 3. 使用

- 运行 MarkdownEditor.exe
- 菜单 View → "AI 助手" 开关
- 输入框打字，Ctrl+Enter 或点"发送"
- 四个快捷按钮可快速填入预设指令

---

## 五、全部文件清单

```
新增文件（10 个）:
  chatbubble.h          chatbubble.cpp
  chatmodel.h           chatmodel.cpp
  aiclient.h            （无 .cpp——纯虚接口）
  deepseekclient.h      deepseekclient.cpp
  contextbuilder.h      contextbuilder.cpp
  chatpanel.h           chatpanel.cpp

修改文件（3 个）:
  mainwindow.h          (+~15 行)
  mainwindow.cpp        (+~85 行)
  CMakeLists.txt        (+5 行)

总计：10 个新文件 + 3 个修改 = 约 1200 行代码
```

---

## 检验清单

1. `m_bubbleLayout->addStretch()` 初始弹簧的作用是什么？新气泡是怎么插入的？
2. `setWidgetResizable(true)` 不设会怎样？
3. 多轮对话的消息数组里，为什么历史最后一条要跳过、重新构造带文档上下文的版本？
4. `m_verticalSplitter` 的两个 stretchFactor 各是多少？总计几份？
5. `toggleAIPanel()` 隐藏 ChatPanel 后空间去哪了？
6. 文档内容是哪个时机传给 ChatPanel 的？
7. CMakeLists.txt 里为什么要加 `Network` 和 `Qt6::Network`？
