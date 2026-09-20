#pragma once

#include <QMainWindow>
#include <QString>

class QPlainTextEdit;
class QPushButton;
class QTextBrowser;
class QListWidget;
class ChatController;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onSendClicked();
    void onOpenSettings();
    void onEditSystemPrompt();

    void onUserMessageAdded(const QString &content, qint64 timestamp);
    void onAiMessageStarted(qint64 timestamp);
    void onAiChunkReceived(const QString &chunk);
    void onAiMessageFinished();
    void onError(const QString &error);

    // 会话相关
    void onNewSession();
    void onDeleteSession();
    void onSessionItemClicked(int row);
    void onSessionItemDoubleClicked(int row);
    void refreshSessionList();
    void onSessionLoaded(const QList<QJsonObject> &history);

private:
    void appendLine(const QString &role, const QString &content,qint64 timestamp = 0);
    void appendToCurrentLine(const QString &text);
    void appendMarkdownMessage(const QString &role,        // 新增
                               const QString &markdown,
                               qint64 timestamp);
    void clearChatView();
    void scrollToBottom();
    void applySettingsToController();

    QPlainTextEdit *m_inputEdit;
    QPushButton    *m_sendButton;
    QPushButton    *m_settingsButton;
    QPushButton    *m_systemPromptButton;
    QPushButton    *m_newSessionButton;
    QPushButton    *m_delSessionButton;
    QTextBrowser   *m_chatView;
    QListWidget    *m_sessionList;
    ChatController *m_controller;
    bool            m_updatingList = false;   // 防止刷新列表时误触发点击槽
};