#include "ChatController.h"
#include "AIProvider.h"
#include "DatabaseManager.h"
#include <QDateTime>
#include <QUuid>

ChatController::ChatController(QObject *parent)
    : QObject(parent)
    , m_provider(new AIProvider(this))
{
    // ---- AI 回调 ----
    connect(m_provider, &AIProvider::chunkReceived,
            this, [this](const QString &chunk) {
                m_pendingAiContent += chunk;
                emit aiChunkReceived(chunk);
            });

    connect(m_provider, &AIProvider::finished,
            this, [this]() {
                Session *s = activeSession();
                if (s && !m_pendingAiContent.isEmpty()) {
                    qint64 ts = m_aiStartedAt;

                    QJsonObject aiMsg;
                    aiMsg["role"]      = "assistant";
                    aiMsg["content"]   = m_pendingAiContent;
                    aiMsg["timestamp"] = QString::number(ts);
                    s->messages.append(aiMsg);

                    DatabaseManager::insertMessage(
                        s->id, "assistant", m_pendingAiContent, ts);
                }
                m_pendingAiContent.clear();
                m_aiStartedAt = 0;
                emit aiMessageFinished();
            });

    connect(m_provider, &AIProvider::errorOccurred,
            this, &ChatController::errorOccurred);

    // ---- 从数据库加载 ----
    m_sessions = DatabaseManager::loadAllSessions();

    if (m_sessions.isEmpty()) {
        // 第一次启动，建个默认会话
        createSession();
    } else {
        m_activeId = m_sessions.first().id;
        emit sessionsChanged();
        emit activeSessionChanged(m_activeId);
    }
}

void ChatController::setApiKey(const QString &key)  { m_provider->setApiKey(key); }
void ChatController::setApiUrl(const QString &url)  { m_provider->setApiUrl(url); }
void ChatController::setModel(const QString &model) { m_provider->setModel(model); }

// ---------- 会话管理 ----------

Session *ChatController::findSession(const QString &id)
{
    for (Session &s : m_sessions) if (s.id == id) return &s;
    return nullptr;
}

const Session *ChatController::findSession(const QString &id) const
{
    for (const Session &s : m_sessions) if (s.id == id) return &s;
    return nullptr;
}

Session *ChatController::activeSession()
{
    return findSession(m_activeId);
}

QString ChatController::createSession()
{
    Session s;
    s.id    = QUuid::createUuid().toString(QUuid::WithoutBraces);
    s.title = QString("新会话 %1").arg(m_sessions.size() + 1);
    m_sessions.append(s);

    // 落盘
    DatabaseManager::insertSession(s);

    emit sessionsChanged();
    switchSession(s.id);
    return s.id;
}

void ChatController::switchSession(const QString &id)
{
    if (!findSession(id)) return;
    if (m_activeId == id) return;

    m_provider->cancel();
    m_activeId = id;

    emit activeSessionChanged(id);
    emit sessionLoaded(sessionMessages(id));
}

void ChatController::deleteSession(const QString &id)
{
    for (int i = 0; i < m_sessions.size(); ++i) {
        if (m_sessions[i].id == id) {
            m_sessions.removeAt(i);
            break;
        }
    }

    // 落盘
    DatabaseManager::deleteSession(id);

    if (m_sessions.isEmpty()) {
        createSession();
        return;
    }

    emit sessionsChanged();

    if (m_activeId == id) {
        switchSession(m_sessions.first().id);
    }
}

void ChatController::renameSession(const QString &id, const QString &title)
{
    Session *s = findSession(id);
    if (!s) return;
    s->title = title.isEmpty() ? "未命名" : title;

    // 落盘
    DatabaseManager::renameSession(id, s->title);

    emit sessionsChanged();
}

QString ChatController::sessionTitle(const QString &id) const
{
    const Session *s = findSession(id);
    return s ? s->title : QString();
}

QList<QJsonObject> ChatController::sessionMessages(const QString &id) const
{
    const Session *s = findSession(id);
    return s ? s->messages : QList<QJsonObject>();
}

QString ChatController::activeSystemPrompt() const
{
    const Session *s = findSession(m_activeId);
    return s ? s->systemPrompt : QString();
}

void ChatController::setActiveSystemPrompt(const QString &prompt)
{
    Session *s = activeSession();
    if (!s) return;
    s->systemPrompt = prompt;
    DatabaseManager::updateSystemPrompt(s->id, prompt);
}

// ---------- 对话 ----------

void ChatController::sendUserMessage(const QString &text)
{
    Session *s = activeSession();
    if (!s) return;

    qint64 now = QDateTime::currentSecsSinceEpoch();

    QJsonObject userMsg;
    userMsg["role"]      = "user";
    userMsg["content"]   = text;
    userMsg["timestamp"] = QString::number(now);
    s->messages.append(userMsg);

    DatabaseManager::insertMessage(s->id, "user", text, now);

    emit userMessageAdded(text, now);

    m_pendingAiContent.clear();
    m_aiStartedAt = now;
    emit aiMessageStarted(now);

    // 组装发给 API 的消息：system 在最前，然后剥掉 timestamp
    QList<QJsonObject> forApi;

    if (!s->systemPrompt.isEmpty()) {
        QJsonObject sys;
        sys["role"]    = "system";
        sys["content"] = s->systemPrompt;
        forApi.append(sys);
    }

    for (const QJsonObject &m : s->messages) {
        QJsonObject copy = m;
        copy.remove("timestamp");
        forApi.append(copy);
    }

    m_provider->send(forApi);
}

void ChatController::clearActiveHistory()
{
    Session *s = activeSession();
    if (!s) return;
    s->messages.clear();
    m_pendingAiContent.clear();
    m_provider->cancel();

    // 落盘
    DatabaseManager::clearMessages(s->id);

    emit sessionLoaded({});
}