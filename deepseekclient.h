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
    void sendMessage(const QJsonArray &messages)override;
    void cancel() override;

    static QString apiKey();
    static void setApiKey(const QString &key);
private slots:
    void onReadyRead();
    void onFinished();
    void onError(QNetworkReply::NetworkError code);
private:
    QByteArray m_ssBuffer;
    QNetworkAccessManager *m_nam;
    QNetworkReply *m_reply;

    static QString s_apiKey;
    static QString configFilePath();
    static void loadApiKey();
    static void saveApiKey(const QString &key);
};

#endif // DEEPSEEKCLIENT_H
