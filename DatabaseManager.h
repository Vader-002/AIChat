#pragma once

#include <QString>
#include <QList>
#include "Session.h"

class DatabaseManager
{
public:
    // 打开数据库、建表，在程序启动时调用一次
    static bool init();

    // 读取所有会话（含各自的消息）
    static QList<Session> loadAllSessions();

    // 会话级操作
    static void insertSession(const Session &s);
    static void renameSession(const QString &id, const QString &title);
    static void deleteSession(const QString &id);

    static void updateSystemPrompt(const QString &id, const QString &prompt);

    // 消息级操作
    static void insertMessage(const QString &sessionId,
                              const QString &role,
                              const QString &content,
                              qint64 createdAt);
    static void clearMessages(const QString &sessionId);

private:
    static QString dbPath();
};