#include "DatabaseManager.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QJsonObject>
#include <QDebug>

static const char *kConnName = "aichat_main";

QString DatabaseManager::dbPath()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);   // 目录不存在就创建
    return dir + "/aichat.db";
}

bool DatabaseManager::init()
{
    if (!QSqlDatabase::contains(kConnName)) {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", kConnName);
        db.setDatabaseName(dbPath());
    }

    QSqlDatabase db = QSqlDatabase::database(kConnName);
    if (!db.open()) {
        qWarning() << "打开数据库失败:" << db.lastError().text();
        return false;
    }

    QSqlQuery q(db);
    if (!q.exec("CREATE TABLE IF NOT EXISTS sessions ("
                "id TEXT PRIMARY KEY,"
                "title TEXT NOT NULL,"
                "system_prompt TEXT DEFAULT '',"
                "created_at INTEGER,"
                "updated_at INTEGER)")) {
        qWarning() << "建表 sessions 失败:" << q.lastError().text();
        return false;
    }

    // 旧库迁移：检查 system_prompt 列是否存在
    bool hasSystemPrompt = false;
    if (q.exec("PRAGMA table_info(sessions)")) {
        while (q.next()) {
            if (q.value(1).toString() == "system_prompt") {
                hasSystemPrompt = true;
                break;
            }
        }
    }
    if (!hasSystemPrompt) {
        if (!q.exec("ALTER TABLE sessions ADD COLUMN system_prompt "
                    "TEXT DEFAULT ''")) {
            qWarning() << "加列 system_prompt 失败:" << q.lastError().text();
        }
    }

    if (!q.exec("CREATE TABLE IF NOT EXISTS messages ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "session_id TEXT NOT NULL,"
                "role TEXT NOT NULL,"
                "content TEXT NOT NULL,"
                "created_at INTEGER)")) {
        qWarning() << "建表 messages 失败:" << q.lastError().text();
        return false;
    }

    q.exec("CREATE INDEX IF NOT EXISTS idx_messages_session "
           "ON messages(session_id)");

    return true;
}

QList<Session> DatabaseManager::loadAllSessions()
{
    QList<Session> result;

    QSqlDatabase db = QSqlDatabase::database(kConnName);
    QSqlQuery q(db);

    if (!q.exec("SELECT id, title, system_prompt FROM sessions "
                "ORDER BY created_at ASC")) {
        qWarning() << "读取会话失败:" << q.lastError().text();
        return result;
    }

    while (q.next()) {
        Session s;
        s.id           = q.value(0).toString();
        s.title        = q.value(1).toString();
        s.systemPrompt = q.value(2).toString();
        result.append(s);
    }
    q.finish();


    for (Session &s : result) {
        QSqlQuery mq(db);
        mq.prepare("SELECT role, content FROM messages "
                   "WHERE session_id = ? ORDER BY id ASC");
        mq.addBindValue(s.id);
        if (!mq.exec()) {
            qWarning() << "读取消息失败:" << mq.lastError().text();
            continue;
        }
        while (mq.next()) {
            QJsonObject obj;
            obj["role"]    = mq.value(0).toString();
            obj["content"] = mq.value(1).toString();
            obj["timestamp"] = QString::number(mq.value(2).toLongLong());
            s.messages.append(obj);
        }
    }

    return result;
}

void DatabaseManager::insertSession(const Session &s)
{
    QSqlDatabase db = QSqlDatabase::database(kConnName);
    QSqlQuery q(db);
    q.prepare("INSERT OR REPLACE INTO sessions"
              "(id, title, system_prompt, created_at, updated_at) "
              "VALUES(?, ?, ?, ?, ?)");
    qint64 now = QDateTime::currentSecsSinceEpoch();
    q.addBindValue(s.id);
    q.addBindValue(s.title);
    q.addBindValue(s.systemPrompt);
    q.addBindValue(now);
    q.addBindValue(now);
    if (!q.exec()) {
        qWarning() << "插入会话失败:" << q.lastError().text();
    }
}

void DatabaseManager::renameSession(const QString &id, const QString &title)
{
    QSqlDatabase db = QSqlDatabase::database(kConnName);
    QSqlQuery q(db);
    q.prepare("UPDATE sessions SET title = ?, updated_at = ? WHERE id = ?");
    q.addBindValue(title);
    q.addBindValue(QDateTime::currentSecsSinceEpoch());
    q.addBindValue(id);
    if (!q.exec()) {
        qWarning() << "重命名会话失败:" << q.lastError().text();
    }
}

void DatabaseManager::updateSystemPrompt(const QString &id,
                                         const QString &prompt)
{
    QSqlDatabase db = QSqlDatabase::database(kConnName);
    QSqlQuery q(db);
    q.prepare("UPDATE sessions SET system_prompt = ?, updated_at = ? "
              "WHERE id = ?");
    q.addBindValue(prompt);
    q.addBindValue(QDateTime::currentSecsSinceEpoch());
    q.addBindValue(id);
    if (!q.exec()) {
        qWarning() << "更新系统提示词失败:" << q.lastError().text();
    }
}

void DatabaseManager::deleteSession(const QString &id)
{
    QSqlDatabase db = QSqlDatabase::database(kConnName);
    QSqlQuery q(db);

    q.prepare("DELETE FROM messages WHERE session_id = ?");
    q.addBindValue(id);
    q.exec();

    q.prepare("DELETE FROM sessions WHERE id = ?");
    q.addBindValue(id);
    if (!q.exec()) {
        qWarning() << "删除会话失败:" << q.lastError().text();
    }
}

void DatabaseManager::insertMessage(const QString &sessionId,
                                    const QString &role,
                                    const QString &content,
                                    qint64 createdAt)
{
    QSqlDatabase db = QSqlDatabase::database(kConnName);
    QSqlQuery q(db);
    q.prepare("INSERT INTO messages(session_id, role, content, created_at) "
              "VALUES(?, ?, ?, ?)");
    q.addBindValue(sessionId);
    q.addBindValue(role);
    q.addBindValue(content);
    q.addBindValue(createdAt);
    if (!q.exec()) {
        qWarning() << "插入消息失败:" << q.lastError().text();
    }
}

void DatabaseManager::clearMessages(const QString &sessionId)
{
    QSqlDatabase db = QSqlDatabase::database(kConnName);
    QSqlQuery q(db);
    q.prepare("DELETE FROM messages WHERE session_id = ?");
    q.addBindValue(sessionId);
    q.exec();
}