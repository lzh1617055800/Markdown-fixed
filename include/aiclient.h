#ifndef AICLIENT_H
#define AICLIENT_H

#include <QObject>
#include <QJsonArray>

class AiClient : public QObject
{
    Q_OBJECT
public:
    explicit AiClient(QObject *parent = nullptr)
        : QObject(parent){}
    virtual ~AiClient() = default;

    virtual void sendMessage(const QJsonArray &messages) = 0;
    virtual void cancel() = 0;

signals:
    void streamChunk(const QString &chunk);
    void responseDone();
    void errorOccurred(const QString &error);
};

#endif // AICLIENT_H
