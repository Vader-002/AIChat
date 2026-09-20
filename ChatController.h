#pragma once

#include <QObject>
#include <QJsonObject>
#include <QList>
#include <QString>
#include "Session.h"

class AIProvider;



class ChatController : public QObject
{
    Q_OBJECT
public:
    explicit ChatController(QObject *parent = nullptr);

    void setApiKey(const QString &key);
    void setApiUrl(const QString &url);
    void setModel(const QString &model);

    // 会话管理
    QString createSession();                  // 返回新会话 id
    void switchSession(const QString &id);
    void deleteSession(const QString &id);
    void renameSession(const QString &id, const QString &title);

    QString activeSystemPrompt() const;
    void setActiveSystemPrompt(const QString &prompt);

    QString activeSessionId() const { return m_activeId; }
    const QList<Session> &sessions() const { return m_sessions; }
    QString sessionTitle(const QString &id) const;
    QList<QJsonObject> sessionMessages(const QString &id) const;

    // 对话
    void sendUserMessage(const QString &text);
    void clearActiveHistory();

signals:
    void sessionsChanged();                          // 列表增删改
    void activeSessionChanged(const QString &id);    // 切换会话
    void sessionLoaded(const QList<QJsonObject> &history);  // 需要重绘整个对话区

    void userMessageAdded(const QString &content, qint64 timestamp);
    void aiMessageStarted(qint64 timestamp);
    void aiChunkReceived(const QString &chunk);
    void aiMessageFinished();
    void errorOccurred(const QString &error);

private:
    Session *findSession(const QString &id);
    const Session *findSession(const QString &id) const;
    Session *activeSession();

    AIProvider *m_provider;
    QList<Session> m_sessions;
    QString m_activeId;
    QString m_pendingAiContent;
    qint64  m_aiStartedAt = 0;
};