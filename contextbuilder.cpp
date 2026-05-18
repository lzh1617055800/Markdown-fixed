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
