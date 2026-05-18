# 第二批：DeepSeekClient / ContextBuilder

**这一批你要创建 2 个模块（4 个文件）。第一批的 AiClient 在这里被继承。**

---

## 前置知识：SSE（Server-Sent Events）协议

DeepSeek 收到你的请求后，不是等全文生成完再一次性发回来，而是生成一个字就发一个字。它怎么发的？用 SSE 格式。

SSE 是 HTTP 流式响应的一种格式。DeepSeek 的返回数据长这样：

```
data: {"id":"cmpl-xxx","choices":[{"index":0,"delta":{"content":"你"}}]}

data: {"id":"cmpl-xxx","choices":[{"index":0,"delta":{"content":"好"}}]}

data: {"id":"cmpl-xxx","choices":[{"index":0,"delta":{"content":"，"}}]}

data: {"id":"cmpl-xxx","choices":[{"index":0,"delta":{"content":"世"}}]}

data: [DONE]
```

每行以 `data: ` 开头，后面是 JSON。最后一行是 `data: [DONE]` 表示结束。两行之间用 `\n` 分隔。

**关键问题**：TCP 是流协议，没有消息边界。你调用 `m_reply->readAll()` 时可能收到：
- 刚好一整行：`data: {...}\n`
- 半行：`data: {"choic`（后半行下次 readyRead 才到）
- 好几行粘在一起：`data: {...}\ndata: {...}\ndata:`

所以不能假设每次 readyRead 收到的都是完整的一行。需要用一个**缓冲区** `m_ssBuffer` 来攒数据，攒够一整行才处理。

---

## 一、DeepSeekClient — AI 调用客户端

### deepseekclient.h — 完整代码

```cpp
#ifndef DEEPSEEKCLIENT_H
#define DEEPSEEKCLIENT_H

#include "aiclient.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>

class DeepSeekClient : public AiClient
{
    Q_OBJECT
public:
    explicit DeepSeekClient(QObject *parent = nullptr);

    void sendMessage(const QJsonArray &messages) override;
    void cancel() override;

    // API Key 的存取（静态，全局共享）
    static QString apiKey();
    static void    setApiKey(const QString &key);

private slots:
    void onReadyRead();
    void onFinished();
    void onError(QNetworkReply::NetworkError code);

private:
    QByteArray m_ssBuffer;              // SSE 行缓冲区

    QNetworkAccessManager *m_nam;       // Qt 的 HTTP 客户端
    QNetworkReply         *m_reply;     // 当前进行中的请求

    static QString s_apiKey;            // 全局 API Key
    static QString configFilePath();    // Key 存储路径
    static void    loadApiKey();
    static void    saveApiKey(const QString &key);
};

#endif // DEEPSEEKCLIENT_H
```

**逐行解释：**

- **`QNetworkAccessManager`**：Qt 的 HTTP 客户端。一个应用只需要一个实例（它内部管理连接池），所以放在 DeepSeekClient 里作为成员变量。

- **`QNetworkReply`**：代表一个进行中的 HTTP 请求/响应。`m_nam->post()` 返回的就是它。通过它的 `readyRead` 信号来异步接收数据。

- **`m_ssBuffer`**：SSE 行缓冲区。原理见上面"前置知识"部分。新数据来了先 `append` 进去，然后一行一行切出来解析。

- **`s_apiKey`**：静态成员变量，整个程序只有一份。这样所有 DeepSeekClient 实例共享同一个 Key。实际上你只会创建一个实例，但做成 static 意味着 API Key 不绑定到任何特定实例。

- **`cancel()`**：用户可能等不及 AI 回复就发下一条消息。cancel 用 `m_reply->abort()` 强行中断 HTTP 请求。

- **三个 slot**：`onReadyRead`（有数据到达）、`onFinished`（请求完成）、`onError`（出错）。它们通过 `connect` 绑定到 `m_reply` 的对应信号上。

---

### deepseekclient.cpp — 完整代码（上）：API Key 管理

```cpp
#include "deepseekclient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrl>
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>

// ═══════════════ API Key 管理 ═══════════════

QString DeepSeekClient::s_apiKey;

QString DeepSeekClient::configFilePath()
{
    // Key 存在 exe 同目录下的 deepseek_key.conf 文件里
    return QCoreApplication::applicationDirPath() + "/deepseek_key.conf";
}

void DeepSeekClient::loadApiKey()
{
    QFile f(configFilePath());
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        s_apiKey = QString::fromUtf8(f.readAll()).trimmed();
        f.close();
    }
}

void DeepSeekClient::saveApiKey(const QString &key)
{
    QFile f(configFilePath());
    if (f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        f.write(key.toUtf8());
        f.close();
    }
    s_apiKey = key;
}

QString DeepSeekClient::apiKey()
{
    if (s_apiKey.isEmpty()) {
        loadApiKey();        // 第一次调用时从文件读取
    }
    return s_apiKey;
}

void DeepSeekClient::setApiKey(const QString &key)
{
    saveApiKey(key);         // 同时写文件和内存
}
```

**逐行解释：**

- **`QCoreApplication::applicationDirPath()`**：返回 exe 所在目录的绝对路径。比如 `D:/project/build/MarkdownEditor.exe` → 返回 `D:/project/build`。Key 文件就放在这个目录下，叫 `deepseek_key.conf`。

- **为什么不存 QSettings？** QSettings 在 Windows 上写注册表（`HKEY_CURRENT_USER\Software\...`），用户想改 Key 要去翻注册表。存成文件，记事本就能改。

- **`loadApiKey()` 用 `QIODevice::Text`**：Windows 上文本模式会自动处理 `\r\n` → `\n` 转换。`.trimmed()` 去掉首尾空白（防止用户多打了空格或换行）。

- **`saveApiKey()` 用 `Truncate`**：如果文件已存在，清空后重写。不用 Append——一个 Key 一行就够了。

- **`QString::fromUtf8()`**：从文件的 UTF-8 字节流转成 QString。Key 通常只有 ASCII 字符，但用 `fromUtf8` 是最安全的选择。

- **静态成员 `s_apiKey` 需要在 .cpp 里定义**：头文件里只是声明（`static QString s_apiKey;`），cpp 里必须写 `QString DeepSeekClient::s_apiKey;` 才能真正分配存储空间。这是 C++ 的规定——静态成员变量在类定义外定义。

---

### deepseekclient.cpp — 完整代码（中）：构造/析构/发送

```cpp
// ═══════════════ 构造 / 析构 ═══════════════

DeepSeekClient::DeepSeekClient(QObject *parent)
    : AiClient(parent)
    , m_reply(nullptr)
{
    m_nam = new QNetworkAccessManager(this);

    // QNetworkAccessManager::finished 信号在新旧 Qt 版本中连接方式不同
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(m_nam, &QNetworkAccessManager::finished,
            this, &DeepSeekClient::onFinished);
#else
    connect(m_nam, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(onFinished()));
#endif
}

// ═══════════════ 发送消息 ═══════════════

void DeepSeekClient::sendMessage(const QJsonArray &messages)
{
    // 1. 如果上一个请求还没完，先取消
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
    m_ssBuffer.clear();

    // 2. 拼请求体 JSON
    QJsonObject body;
    body["model"]    = QStringLiteral("deepseek-chat");
    body["messages"] = messages;
    body["stream"]   = true;            // ← 关键：开启流式模式

    QJsonDocument doc(body);

    // 3. 构造 HTTP 请求头
    QNetworkRequest req(QUrl("https://api.deepseek.com/v1/chat/completions"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization",
                     ("Bearer " + apiKey()).toUtf8());

    // 4. 发送 POST（异步！不阻塞 UI 线程）
    m_reply = m_nam->post(req, doc.toJson(QJsonDocument::Compact));

    // 5. 绑定回调
    connect(m_reply, &QNetworkReply::readyRead,
            this, &DeepSeekClient::onReadyRead);

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(m_reply, &QNetworkReply::errorOccurred,
            this, &DeepSeekClient::onError);
#else
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)),
            this, SLOT(onError(QNetworkReply::NetworkError)));
#endif
}

void DeepSeekClient::cancel()
{
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
    m_ssBuffer.clear();
}
```

**逐行解释：**

- **`body["stream"] = true`**：这是整个打字机效果的总开关。如果写成 `false`，DeepSeek 会等全文生成完再一次性返回——打字机效果就没了。`stream: true` 告诉 API："不要等生成完，生成一个字就发一个字"。

- **`QJsonDocument::Compact`**：JSON 格式化方式。`Compact` 去掉所有空格和换行（比如 `{"a":1}`），`Indented` 保留缩进（带换行和空格）。对 HTTP 请求体来说，Compact 更省字节。

- **`setRawHeader` vs `setHeader`**：`setRawHeader` 直接写入原始字节到 HTTP 头，不做任何处理。`Authorization` 头用 `setRawHeader` 是因为 Qt 对某些标准头（包括 Authorization）有特殊处理逻辑，用 `setHeader` 可能被改写或过滤掉。

- **`"Bearer " + apiKey()`**：HTTP Bearer Token 认证标准。格式是 `Authorization: Bearer sk-xxxxx`。中间有一个空格，别漏了。

- **`m_nam->post(req, body)`**：**这是异步的！** Qt 立即返回 QNetworkReply 指针，然后后台处理 DNS 解析、TCP 连接、TLS 握手、发送请求、接收响应。你的 UI 线程完全不阻塞，用户可以继续编辑文档。

- **`if (m_reply) { abort(); }`**：用户可能连按两次发送。如果不取消上一次请求，两个请求的响应会搅在一起（SSBuffer 里就会有两种不同的 SSE 流）。先 abort 再发新的，保证每次只有一个请求在飞。

- **`deleteLater()` 而不是 `delete`**：`deleteLater()` 把删除操作投递到 Qt 事件队列末尾，等当前正在处理的所有事件（包括触发当前函数的 readyRead 信号）完成后才真正 delete。用 `delete` 可能删掉正在发信号的对象的子对象，导致崩溃。

- **Qt 版本兼容 `#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)`**：Qt 5.15 把 `QNetworkReply::error()` 信号重命名为 `errorOccurred()`，因为 `error` 和父类的 `error()` 函数名冲突。你的 Qt 6.10 一定是新版，用 `&QNetworkReply::errorOccurred`。保留旧版宏只是好习惯。

---

### deepseekclient.cpp — 完整代码（下）：SSE 解析

```cpp
// ═══════════════ SSE 流解析 ═══════════════

void DeepSeekClient::onReadyRead()
{
    if (!m_reply) return;

    // 第一步：把新到的数据追加到缓冲区
    m_ssBuffer.append(m_reply->readAll());

    // 第二步：按行解析
    while (true) {
        int idx = m_ssBuffer.indexOf('\n');
        if (idx == -1) break;          // 没找到完整行，等下次 readyRead

        QByteArray line = m_ssBuffer.left(idx).trimmed();
        m_ssBuffer.remove(0, idx + 1); // 从缓冲区删掉已处理的行

        if (line.isEmpty()) continue;
        if (!line.startsWith("data: ")) continue;

        if (line == "data: [DONE]") {
            m_ssBuffer.clear();
            emit responseDone();
            return;
        }

        // 提取 "data: " 后面的 JSON
        QByteArray jsonBytes = line.mid(6);  // 跳过前 6 个字符 "data: "
        QJsonParseError parseErr;
        QJsonDocument jdoc = QJsonDocument::fromJson(jsonBytes, &parseErr);
        if (parseErr.error != QJsonParseError::NoError) continue;

        QJsonObject root = jdoc.object();
        QJsonArray choices = root["choices"].toArray();
        if (choices.isEmpty()) continue;

        QJsonObject firstChoice = choices[0].toObject();
        QJsonObject delta = firstChoice["delta"].toObject();
        QString content = delta["content"].toString();

        if (!content.isEmpty()) {
            emit streamChunk(content);
        }
    }
}

void DeepSeekClient::onFinished()
{
    if (!m_reply) return;

    // 处理 SSBuffer 里可能残留的最后一段数据
    onReadyRead();

    // 如果处理完还有非空白残留，说明格式异常
    if (!m_ssBuffer.trimmed().isEmpty()) {
        emit errorOccurred(QStringLiteral("响应解析异常: ")
                           + QString::fromUtf8(m_ssBuffer.left(200)));
    }

    m_reply->deleteLater();
    m_reply = nullptr;
}

void DeepSeekClient::onError(QNetworkReply::NetworkError code)
{
    Q_UNUSED(code)
    if (m_reply) {
        QString errMsg = m_reply->errorString();
        QByteArray respBody = m_reply->readAll();
        if (!respBody.isEmpty()) {
            errMsg += "\n" + QString::fromUtf8(respBody);
        }
        emit errorOccurred(errMsg);
        m_reply->deleteLater();
        m_reply = nullptr;
    }
}
```

**逐行解释：**

- **`m_ssBuffer.indexOf('\n')`**：在缓冲区里找第一个换行符。找到了→说明至少有一整行，切出来处理。找不到→还没攒够一行，break 退出等下次 readyRead。

- **`m_ssBuffer.left(idx)`**：取缓冲区前 idx 个字节（从开头到 `\n` 之前）。这是完整的一行。

- **`m_ssBuffer.remove(0, idx + 1)`**：把已处理的行（包括 `\n`）从缓冲里删掉。`+1` 是因为要去掉 `\n` 本身。

- **`.trimmed()`**：去掉行末可能的 `\r`（Windows 换行是 `\r\n`，`\n` 被 indexOf 找到了，`\r` 还留在 left 的结果里）。

- **`line.mid(6)`**：跳过 `"data: "` 这 6 个字符，取后面的 JSON 部分。`mid(6)` = 从索引 6 开始到末尾。

- **`QJsonDocument::fromJson`**：用 QJsonParseError 参数可以判断解析是否成功。如果 API 返回的 JSON 格式有问题（极少见但理论可能存在），跳过这一行而不是崩溃。

- **JSON 路径 `root["choices"][0]["delta"]["content"]`**：这是 OpenAI 兼容 API 的标准响应结构。`choices` 是数组（通常只有一个元素），`delta.content` 是这次新增的文本片段。

- **`onFinished` 里为什么又调了一次 `onReadyRead()`？** 网络层通知顺序不确定。可能 `finished` 信号到了但 `readyRead` 还没被调用完。手动再调一次确保缓冲区不剩东西。

- **`m_ssBuffer.left(200)`**：错误消息只截取前 200 字节，防止 API 返回一个巨大的 HTML 错误页面撑爆 UI。

---

## 二、ContextBuilder — 拼 AI 上下文

### 它干什么

把"编辑器内容 + 用户问题 + 系统角色设定"拼成发给 AI 的 messages 数组。

### contextbuilder.h — 完整代码

```cpp
#ifndef CONTEXTBUILDER_H
#define CONTEXTBUILDER_H

#include <QJsonArray>
#include <QString>

class ContextBuilder
{
public:
    // 完整上下文：system prompt + 文档全文 + 用户消息
    static QJsonArray build(const QString &documentContent,
                            const QString &userMessage);

    // 系统提示词（角色设定）
    static QString systemPrompt();

    // 四个快捷按钮的预设提示词
    static QString polishPrompt();     // 润色
    static QString expandPrompt();     // 扩写
    static QString translatePrompt();  // 翻译英文
    static QString summarizePrompt();  // 总结
};

#endif // CONTEXTBUILDER_H
```

### contextbuilder.cpp — 完整代码

```cpp
#include "contextbuilder.h"
#include <QJsonObject>

QJsonArray ContextBuilder::build(const QString &documentContent,
                                  const QString &userMessage)
{
    QJsonArray arr;

    // 第 1 条：system 消息（AI 的角色设定）
    QJsonObject sysMsg;
    sysMsg["role"]    = QStringLiteral("system");
    sysMsg["content"] = systemPrompt();
    arr.append(sysMsg);

    // 第 2 条：user 消息（文档全文 + 用户问题）
    QString userContent;
    if (documentContent.isEmpty()) {
        userContent = userMessage;
    } else {
        userContent = QStringLiteral(
            "我正在编辑以下 Markdown 文档:\n\n"
            "```markdown\n%1\n```\n\n"
            "%2"
        ).arg(documentContent, userMessage);
    }

    QJsonObject userMsg;
    userMsg["role"]    = QStringLiteral("user");
    userMsg["content"] = userContent;
    arr.append(userMsg);

    return arr;
}

QString ContextBuilder::systemPrompt()
{
    return QStringLiteral(
        "你是 LightMark 的 AI 写作助手，内置于一款 Markdown 编辑器中。"
        "你的任务是帮助用户提升文档质量。\n\n"
        "能力范围：\n"
        "- 润色文本：优化表达、调整语气、修正语法错误\n"
        "- 扩写内容：在保持原意的基础上丰富细节\n"
        "- 翻译：将中文翻译为英文，或将英文翻译为中文\n"
        "- 总结：提取文档的核心要点\n"
        "- 回答 Markdown 语法问题\n"
        "- 对用户的写作提出建议\n\n"
        "规则：\n"
        "- 用中文回复，除非用户要求用英文\n"
        "- 回复简洁，不要过度冗长\n"
        "- 如果用户要求修改某段文字，直接给出修改后的版本"
    );
}

// ═══════════════ 快捷按钮提示词 ═══════════════

QString ContextBuilder::polishPrompt()
{
    return QStringLiteral("请帮我润色以下文字，优化表达和语法，保持原意不变：\n\n");
}

QString ContextBuilder::expandPrompt()
{
    return QStringLiteral("请帮我扩写以下内容，丰富细节和例证，保持原有风格：\n\n");
}

QString ContextBuilder::translatePrompt()
{
    return QStringLiteral("请帮我将以下内容翻译为英文：\n\n");
}

QString ContextBuilder::summarizePrompt()
{
    return QStringLiteral("请帮我总结以下内容的核心要点：\n\n");
}
```

**逐行解释：**

- **system prompt 的重要性**：这是你给 AI 的"角色设定"。你写"你是写作助手，擅长 Markdown"，AI 就会以写作助手身份回答。你写"你是 Linux 命令行专家"，它就会给你敲 bash 命令。system prompt 决定了 AI 的"人设"。

- **为什么用 `\`\`\`markdown 代码块` 包裹文档？** Prompt Engineering 小技巧——用代码块区分"文档内容"和"用户指令"，AI 更不容易混淆。类比：你写代码时用 `/* 注释 */` 标注注释，AI 也需要标记来区分不同语义区域。

- **`build()` 只发了 2 条消息（system + user），多轮对话的历史怎么办？** 历史消息由 ChatPanel 的 `onSendClicked()` 在调用 `build()` 之前手工拼好。ContextBuilder 只负责一次发送的"系统消息 + 当前用户消息"，历史消息是 ChatPanel 从 ChatModel 里取出来插入到中间的。

- **四个快捷提示词**：每个后缀 `\n\n`——两个换行，让用户可以直接粘贴要处理的文本，光标在下一行等着。比如点"润色"后输入框里是：`请帮我润色以下文字...\n\n|`（光标在 | 处）。

- **为什么用 `static` 方法？** ContextBuilder 没有状态——它就是一个工具函数。做成静态方法比实例化一个对象更直接。

---

## 检验清单

1. SSE 格式里 `data: [DONE]` 是什么意思？
2. `m_ssBuffer` 的作用是什么？如果没有它会发生什么？
3. `body["stream"] = true` 设成 `false`，效果有什么不同？
4. `m_reply->deleteLater()` 为什么不能用 `delete m_reply` 替代？
5. `"Bearer " + apiKey()` 中间那个空格的作用是什么？
6. system prompt 写了什么？如果把它改成"你是 Linux 专家"，会发生什么？
7. `\n\n` 后缀在四个快捷提示词里有什么用？
