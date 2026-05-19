#include "ChatModel.h"

void ChatModel::addMessage(Role role,const QString &content)
{
    m_messages.append({role,content,QDateTime::currentDateTime()});
}

void ChatModel::clear()
{
    m_messages.clear();
}

QJsonArray ChatModel::toJsonArray()const
{
    QJsonArray arr;
    for(const auto &msg : m_messages)
    {
        QJsonObject obj;
        obj["role"] = roleToString(msg.role);
        obj["content"] = msg.content;
        arr.append(obj);
    }
    return arr;
}

QString ChatModel::roleToString(Role role)
{
    switch(role)
    {
    case User: return QStringLiteral("user");
    case Assistant: return QStringLiteral("assistant");
    case System: return QStringLiteral("system");
    }
    return QStringLiteral("user");
}
