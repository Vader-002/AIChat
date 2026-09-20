#pragma once

#include <QObject>
#include <QByteArray>
#include <QJsonObject>
#include <QList>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

class AIProvider : public QObject
{
    Q_OBJECT
public:
    explicit AIProvider(QObject *parent = nullptr);
    ~AIProvider();

    void setApiKey(const QString &key);
    void setApiUrl(const QString &url);
    void setModel(const QString &model);

    void send(const QList<QJsonObject> &messages);
    void cancel();

signals:
    void chunkReceived(const QString &chunk);
    void finished();
    void errorOccurred(const QString &error);

private slots:
    void onReadyRead();
    void onReplyFinished();

private:
    QNetworkAccessManager *m_net;
    QNetworkReply *m_reply = nullptr;
    QByteArray m_buffer;

    QString m_apiKey;
    QString m_apiUrl;
    QString m_model;
};