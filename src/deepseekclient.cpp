#include "deepseekclient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrl>
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>

QString DeepSeekClient::s_apiKey;

QString DeepSeekClient::configFilePath()
{
    return QCoreApplication::applicationDirPath()+"/deepseek_key.conf";
}

void DeepSeekClient::loadApiKey()
{
    QFile f(configFilePath());
    if(f.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        s_apiKey = QString::fromUtf8(f.readAll()).trimmed();
        f.close();
    }
}

void DeepSeekClient::saveApiKey(const QString &key)
{
    QFile f(configFilePath());
    if (f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        f.write(key.toUtf8());
        f.close();
    }
    s_apiKey = key;
}

QString DeepSeekClient::apiKey()
{
    if(s_apiKey.isEmpty())
    {
        loadApiKey();
    }
    return s_apiKey;
}

void DeepSeekClient::setApiKey(const QString &key)
{
    saveApiKey(key);
}

DeepSeekClient::DeepSeekClient(QObject *parent)
    :AiClient(parent),m_reply(nullptr)
{
    m_nam =  new QNetworkAccessManager(this);
    connect(m_nam,&QNetworkAccessManager::finished,this,&DeepSeekClient::onFinished);
}


void DeepSeekClient::sendMessage(const QJsonArray &messages)
{
    if(m_reply)
    {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
    m_ssBuffer.clear();

    QJsonObject body;
    body["model"] = QStringLiteral("deepseek-chat");
    body["messages"] = messages;
    body["stream"] = true;//打字机效果有字就发

    QJsonDocument doc(body);

    QNetworkRequest req(QUrl("https://api.deepseek.com/v1/chat/completions"));
    req.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    req.setRawHeader("Authorization",("Bearer " + apiKey()).toUtf8());

    m_reply = m_nam->post(req,doc.toJson(QJsonDocument::Compact));//异步发送防止阻塞UI

    connect(m_reply,&QNetworkReply::readyRead,this,&DeepSeekClient::onReadyRead);
    connect(m_reply, &QNetworkReply::errorOccurred,this, &DeepSeekClient::onError);

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

void DeepSeekClient::onReadyRead()
{
    if(!m_reply)
    {
        return;
    }

    m_ssBuffer.append(m_reply->readAll());
    while(true)
    {
        int idx = m_ssBuffer.indexOf('\n');
        if(idx == -1)
        {
            break;
        }
        //trimmed()` 去掉首尾空白（防止用户多打了空格或换行）
        QByteArray line = m_ssBuffer.left(idx).trimmed();
        m_ssBuffer.remove(0,idx + 1);//去掉取出的内容，包括换行符

        if(line.isEmpty())
        {
            continue;
        }
        if(!line.startsWith("data: ")) continue;

        if(line == "data: [DONE]")
        {
            m_ssBuffer.clear();
            emit responseDone();
            return;
        }

        QByteArray jsonBytes = line.mid(6);
        QJsonParseError parseErr;
        QJsonDocument jdoc = QJsonDocument::fromJson(jsonBytes,&parseErr);
        if(parseErr.error != QJsonParseError::NoError) continue;

        QJsonObject root = jdoc.object();
        QJsonArray choices = root["choices"].toArray();
        if(choices.isEmpty()) continue;

        QJsonObject firstChoice = choices[0].toObject();
        QJsonObject delta = firstChoice["delta"].toObject();
        QString content = delta["content"].toString();

        if(!content.isEmpty())
        {
            emit streamChunk(content);
        }

    }
}

void DeepSeekClient::onFinished()
{
    if(!m_reply) return;

    onReadyRead();

    if(!m_ssBuffer.trimmed().isEmpty())
    {
        emit errorOccurred(QStringLiteral("响应解析异常") + QString::fromUtf8(m_ssBuffer.left(200)));
    }

    m_reply->deleteLater();
    m_reply = nullptr;
}

void DeepSeekClient::onError(QNetworkReply::NetworkError code)
{
    Q_UNUSED(code);
    if(m_reply)
    {
        QString errMsg = m_reply->errorString();
        QByteArray respBody = m_reply->readAll();
        if(!respBody.isEmpty())
        {
            errMsg += "\n" + QString::fromUtf8(respBody);
        }
        emit errorOccurred(errMsg);
        m_reply->deleteLater();
        m_reply = nullptr;
    }
}
