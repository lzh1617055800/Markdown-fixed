#ifndef CONTEXTBUILDER_H
#define CONTEXTBUILDER_H
//将编辑器内容+用户问题+系统角色设定，拼成message数组发给AI
#include <QJsonArray>
#include <QString>

class ContextBuilder
{
public:
    static QJsonArray build(const QString &documentContent,const QString &userMessage);
    static QString systemPrompt();//系统提示词

    static QString polishPrompt();
    static QString expandPrompt();
    static QString translatePrompt();
    static QString summarizePrompt();
};

#endif // CONTEXTBUILDER_H
