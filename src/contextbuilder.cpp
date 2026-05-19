#include "contextbuilder.h"
#include <QJsonObject>

QJsonArray ContextBuilder::build(const QString &documentContent,const QString &userMessage )
{
    QJsonArray arr;

    QJsonObject sysMsg;
    sysMsg["role"] = QStringLiteral("system");
    sysMsg["content"] = systemPrompt();
    arr.append(sysMsg);


    QString userContent;
    if(documentContent.isEmpty())
    {
        userContent = userMessage;
    }
    else
    {
        userContent = QStringLiteral(
            "我正在编辑一下Markdown文档:\n\n"
            "```markdown\n%1\n```\n\n"
            "%2"
        ).arg(documentContent,userMessage);
    }

    QJsonObject userMsg;
    userMsg["role"] = QStringLiteral("user");
    userMsg["content"] = userContent;
    arr.append(userMsg);

    return arr;
}

QString ContextBuilder::systemPrompt()
{
    return QStringLiteral(
        "你是 LightMark 的 AI 助手，内置于一款 Markdown 编辑器中，底层是deepseek-v4-falsh。"
        "你可以帮用户做任何事：写作、编程、回答技术问题、分析文档、讨论行业话题。\n\n"
        "你的知识截止到 2026 年初。对于这个时间点之前的事实性问题，直接回答。"
        "对于你不知道的近期事件，诚实说不知道，然后提供你已有的相关知识帮助用户思考。\n\n"
        "规则：\n"
        "- 用中文回复，除非用户要求用其他语言\n"
        "- 回复直接、有信息量，不要堆砌免责声明\n"
        "- 如果用户要求修改文档，直接给出修改后的版本"
        );
}

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
