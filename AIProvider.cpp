#include "AIProvider.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QUrl>

AIProvider::AIProvider(QObject *parent)
    : QObject(parent), m_net(new QNetworkAccessManager(this))
{
}

AIProvider::~AIProvider()
{
    cancel();
}

void AIProvider::setApiKey(const QString &key)   { m_apiKey = key; }
void AIProvider::setApiUrl(const QString &url)   { m_apiUrl = url; }
void AIProvider::setModel(const QString &model)  { m_model  = model; }

void AIProvider::send(const QList<QJsonObject> &messages)
{
    cancel();   // 如果上一次还没结束，先取消

    QJsonObject root;
    root["model"]  = m_model;
    root["stream"] = true;

    QJsonArray arr;
    for (const QJsonObject &m : messages) {
        arr.append(m);
    }
    root["messages"] = arr;

    QJsonDocument doc(root);

    QNetworkRequest req{QUrl(m_apiUrl)};
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", "Bearer " + m_apiKey.toUtf8());

    m_buffer.clear();
    m_reply = m_net->post(req, doc.toJson());

    connect(m_reply, &QNetworkReply::readyRead,
            this, &AIProvider::onReadyRead);
    connect(m_reply, &QNetworkReply::finished,
            this, &AIProvider::onReplyFinished);
}

void AIProvider::cancel()
{
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
}

void AIProvider::onReadyRead()
{
    if (!m_reply) return;

    m_buffer.append(m_reply->readAll());
    m_buffer.replace("\r\n", "\n");

    while (true) {
        int idx = m_buffer.indexOf("\n\n");
        if (idx < 0) break;

        QByteArray event = m_buffer.left(idx);
        m_buffer.remove(0, idx + 2);

        for (const QByteArray &line : event.split('\n')) {
            if (!line.startsWith("data: ")) continue;

            QByteArray data = line.mid(6).trimmed();
            if (data == "[DONE]") {
                return;   // 结束交给 onReplyFinished 统一处理
            }

            QJsonDocument d = QJsonDocument::fromJson(data);
            QJsonObject obj = d.object();
            QJsonArray choices = obj["choices"].toArray();
            if (choices.isEmpty()) continue;

            QJsonObject delta = choices[0].toObject()["delta"].toObject();
            if (delta.contains("content")) {
                QString chunk = delta["content"].toString();
                if (!chunk.isEmpty()) {
                    emit chunkReceived(chunk);
                }
            }
        }
    }
}

void AIProvider::onReplyFinished()
{
    if (!m_reply) return;

    if (m_reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(m_reply->errorString());
    }

    m_reply->deleteLater();
    m_reply = nullptr;

    emit finished();
}