# LightMark AI 助手设计文档

## 目标

在 LightMark 编辑器底部新增 AI 写作助手面板，接入 DeepSeek API，实现一问一答的自由对话。

## 后端

DeepSeek API（OpenAI 兼容格式），流式 SSE 返回，国内直连。

## 新增模块

### 1. ChatBubble（chatbubble.h/cpp）
- 单个对话气泡 QWidget
- 用户气泡：蓝色背景、靠右
- AI 气泡：灰色背景、靠左
- 状态：waiting（显示"..."动画）→ streaming（逐字追加）→ done
- 内部用 QLabel 展示文本

### 2. ChatPanel（chatpanel.h/cpp）
- 底部面板 QWidget，内部布局：
  - 快捷按钮栏（"润色" "扩写" "翻译英文" "总结"）
  - QScrollArea（包含多个 ChatBubble）
  - 输入行（QTextEdit + 发送按钮）
- Ctrl+Enter 发送
- 快捷按钮点后自动在输入框填提示词，仍需手动发送

### 3. ChatModel（chatmodel.h/cpp）
- 纯数据类，不依赖 UI
- Message: enum Role {User, Assistant, System} + content + timestamp
- 消息列表 QVector<Message>
- toJsonArray() 输出 OpenAI 格式的 messages 数组

### 4. AiClient（aiclient.h）
- 纯虚基类，定义接口
- 信号：streamChunk(QString chunk) / responseDone() / errorOccurred(QString)
- 虚函数：sendMessage(QJsonArray messages) / cancel()

### 5. DeepSeekClient（deepseekclient.h/cpp）
- 继承 AiClient
- 用 QNetworkAccessManager 发 HTTPS POST 到 api.deepseek.com/v1/chat/completions
- 请求体：{model:"deepseek-chat", messages:[...], stream:true}
- 解析 SSE 响应：data: {"choices":[{"delta":{"content":"文本"}}]}
- 每次 readyRead 触发时增量解析，emit streamChunk
- API Key 从 QSettings 读取（首次运行时弹出对话框让用户输入）

### 6. ContextBuilder（contextbuilder.h/cpp）
- 静态方法：build(QString docContent, QString userMsg) → QJsonArray
- system 消息："你是 LightMark 的 AI 写作助手，擅长 Markdown 文档写作..."
- user 消息：拼接"当前文档内容:\n{docContent}\n\n用户问题:\n{userMsg}"

## MainWindow 改动

- setupUI()：新增纵向 QSplitter，上方原 3 栏，下方 ChatPanel
- View 菜单加"AI 助手"开关
- 成员变量：ChatPanel* / DeepSeekClient*
- 连线：ChatPanel 发送信号 → ContextBuilder → DeepSeekClient → streamChunk → ChatPanel 更新气泡

## 数据流

```
用户按发送
  → ChatPanel 创建用户气泡(user) + AI气泡(waiting)
  → ContextBuilder::build(编辑器全文, 用户输入)
  → DeepSeekClient::sendMessage(messages)
  → QNetworkAccessManager POST https://api.deepseek.com/v1/chat/completions
  → SSE 流式返回，readyRead 触发
  → 解析 delta.content → emit streamChunk(chunk)
  → ChatPanel::onStreamChunk 追加文本到 AI 气泡
  → 流结束 → emit responseDone()
  → ChatPanel::onResponseDone 标记完成
```

## 文件清单

```
新增:
  src/chatbubble.h / .cpp        (~200行)
  src/chatpanel.h / .cpp         (~350行)
  src/chatmodel.h / .cpp         (~150行)
  src/aiclient.h                 (~40行, 纯虚接口)
  src/deepseekclient.h / .cpp    (~250行)
  src/contextbuilder.h / .cpp    (~100行)

修改:
  src/mainwindow.h               (+~15行)
  src/mainwindow.cpp             (+~80行)
  CMakeLists.txt                 (+~5行, 加 network 模块)
```

预估代码量：~1200 行

## Qt 模块依赖

需要在 CMakeLists.txt 的 `find_package` 里加上 `Network` 组件：
```cmake
find_package(Qt6 REQUIRED COMPONENTS Core Widgets PrintSupport Network)
```
