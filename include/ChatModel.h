#ifndef CHATMODEL_H
#define CHATMODEL_H

#include <QString>
#include <QDateTime>
#include <QVector>
#include <QJsonArray>
#include <QJsonObject>

class ChatModel{
public:
    enum Role{User,Assistant,System};
    struct Message
    {
        Role role;
        QString content;
        QDateTime timestamp;
    };
    void addMessage(Role role,const QString &content);
    const QVector<Message>& messages()const {return m_messages;}
    void clear();

    QJsonArray toJsonArray() const;
private:
    QVector<Message> m_messages;
    static QString roleToString(Role role);
};

#endif // CHATMODEL_H
