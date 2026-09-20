#include "mainwindow.h"
#include "ChatController.h"
#include "SettingsManager.h"
#include "SettingsDialog.h"


#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTextBrowser>
#include <QListWidget>
#include <QScrollBar>
#include <QTextCursor>
#include <QMessageBox>
#include <QInputDialog>
#include <QJsonObject>
#include <QDateTime>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QDateTime>
#include <QDate>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_inputEdit(nullptr)
    , m_sendButton(nullptr)
    , m_settingsButton(nullptr)
    , m_systemPromptButton(nullptr)
    , m_newSessionButton(nullptr)
    , m_delSessionButton(nullptr)
    , m_chatView(nullptr)
    , m_sessionList(nullptr)
    , m_controller(new ChatController(this))
{
    setWindowTitle("AI Chat");

    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *outerLayout = new QHBoxLayout(central);
    auto *splitter = new QSplitter(Qt::Horizontal, this);
    outerLayout->addWidget(splitter);

    // ===== 左侧：会话列表 =====
    auto *leftWidget = new QWidget(this);
    auto *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    auto *leftTop = new QHBoxLayout();
    m_newSessionButton = new QPushButton("新建", this);
    m_delSessionButton = new QPushButton("删除", this);
    leftTop->addWidget(m_newSessionButton);
    leftTop->addWidget(m_delSessionButton);
    leftLayout->addLayout(leftTop);

    m_sessionList = new QListWidget(this);
    leftLayout->addWidget(m_sessionList, 1);

    splitter->addWidget(leftWidget);

    // ===== 右侧：聊天区 =====
    auto *rightWidget = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    // 顶部工具栏
    auto *topLayout = new QHBoxLayout();
    topLayout->addStretch();
    m_systemPromptButton = new QPushButton("系统提示词", this);
    topLayout->addWidget(m_systemPromptButton);
    m_settingsButton = new QPushButton("设置", this);
    topLayout->addWidget(m_settingsButton);
    rightLayout->addLayout(topLayout);

    // 聊天显示
    m_chatView = new QTextBrowser(this);
    m_chatView->setReadOnly(true);
    m_chatView->setOpenExternalLinks(true);
    rightLayout->addWidget(m_chatView, 1);

    // 输入区
    auto *inputLayout = new QHBoxLayout();
    m_inputEdit  = new QPlainTextEdit(this);
    m_sendButton = new QPushButton("发送", this);
    m_inputEdit->setPlaceholderText("输入消息……");
    m_inputEdit->setFixedHeight(80);
    inputLayout->addWidget(m_inputEdit);
    inputLayout->addWidget(m_sendButton);
    rightLayout->addLayout(inputLayout);

    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({200, 600});

    // ===== 信号连接 =====
    connect(m_sendButton, &QPushButton::clicked,
            this, &MainWindow::onSendClicked);
    connect(m_settingsButton, &QPushButton::clicked,
            this, &MainWindow::onOpenSettings);
    connect(m_systemPromptButton, &QPushButton::clicked,
            this, &MainWindow::onEditSystemPrompt);
    connect(m_newSessionButton, &QPushButton::clicked,
            this, &MainWindow::onNewSession);
    connect(m_delSessionButton, &QPushButton::clicked,
            this, &MainWindow::onDeleteSession);

    connect(m_sessionList, &QListWidget::currentRowChanged,
            this, &MainWindow::onSessionItemClicked);
    connect(m_sessionList, &QListWidget::itemDoubleClicked,
            this, [this](QListWidgetItem *) {
                onSessionItemDoubleClicked(m_sessionList->currentRow());
            });

    // controller → 界面
    connect(m_controller, &ChatController::sessionsChanged,
            this, &MainWindow::refreshSessionList);
    connect(m_controller, &ChatController::sessionLoaded,
            this, &MainWindow::onSessionLoaded);
    connect(m_controller, &ChatController::userMessageAdded,
            this, &MainWindow::onUserMessageAdded);
    connect(m_controller, &ChatController::aiMessageStarted,
            this, &MainWindow::onAiMessageStarted);
    connect(m_controller, &ChatController::aiChunkReceived,
            this, &MainWindow::onAiChunkReceived);
    connect(m_controller, &ChatController::errorOccurred,
            this, &MainWindow::onError);
    connect(m_controller, &ChatController::aiMessageFinished,
            this, &MainWindow::onAiMessageFinished);

    // 首次配置
    if (SettingsManager::apiKey().isEmpty()) {
        QMessageBox::information(this, "首次使用",
                                 "还没有配置 API Key，请先在设置里填写。");
        onOpenSettings();
    }
    applySettingsToController();

    refreshSessionList();

    resize(950, 650);
}

MainWindow::~MainWindow() {}

// ---------- 发送 / 设置 ----------

void MainWindow::onSendClicked()
{
    QString text = m_inputEdit->toPlainText().trimmed();
    if (text.isEmpty()) return;

    m_inputEdit->clear();
    m_controller->sendUserMessage(text);
}

void MainWindow::onOpenSettings()
{
    SettingsDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        applySettingsToController();
    }
}

void MainWindow::onEditSystemPrompt()
{
    QString current = m_controller->activeSystemPrompt();

    bool ok = false;
    QString text = QInputDialog::getMultiLineText(
        this,
        "系统提示词",
        "为该会话设定 AI 的角色（留空则不使用）：",
        current, &ok);

    if (ok) {
        m_controller->setActiveSystemPrompt(text.trimmed());
    }
}

void MainWindow::applySettingsToController()
{
    m_controller->setApiKey(SettingsManager::apiKey());
    m_controller->setApiUrl(SettingsManager::apiUrl());
    m_controller->setModel (SettingsManager::model());
}

// ---------- 会话列表 ----------

void MainWindow::onNewSession()
{
    m_controller->createSession();
}

void MainWindow::onDeleteSession()
{
    QString id = m_controller->activeSessionId();
    if (id.isEmpty()) return;

    QString title = m_controller->sessionTitle(id);
    auto ret = QMessageBox::question(this, "删除会话",
                                     QString("确定删除「%1」吗？").arg(title));
    if (ret == QMessageBox::Yes) {
        m_controller->deleteSession(id);
    }
}

void MainWindow::onSessionItemClicked(int row)
{
    if (m_updatingList) return;
    if (row < 0) return;

    auto *item = m_sessionList->item(row);
    if (!item) return;

    QString id = item->data(Qt::UserRole).toString();
    m_controller->switchSession(id);
}

void MainWindow::onSessionItemDoubleClicked(int row)
{
    if (row < 0) return;
    auto *item = m_sessionList->item(row);
    if (!item) return;

    QString id   = item->data(Qt::UserRole).toString();
    QString name = item->text();

    bool ok = false;
    QString newName = QInputDialog::getText(
        this, "重命名会话", "会话名：",
        QLineEdit::Normal, name, &ok);
    if (ok && !newName.trimmed().isEmpty()) {
        m_controller->renameSession(id, newName.trimmed());
    }
}

void MainWindow::refreshSessionList()
{
    m_updatingList = true;
    m_sessionList->clear();

    QString activeId = m_controller->activeSessionId();
    const auto &sessions = m_controller->sessions();

    int activeRow = -1;
    for (int i = 0; i < sessions.size(); ++i) {
        auto *item = new QListWidgetItem(sessions[i].title, m_sessionList);
        item->setData(Qt::UserRole, sessions[i].id);
        if (sessions[i].id == activeId) {
            activeRow = i;
        }
    }

    if (activeRow >= 0) {
        m_sessionList->setCurrentRow(activeRow);
    }
    m_updatingList = false;
}

void MainWindow::onSessionLoaded(const QList<QJsonObject> &history)
{
    clearChatView();

    for (const QJsonObject &m : history) {
        QString role    = m["role"].toString();
        QString content = m["content"].toString();

        qint64 ts = 0;
        if (m.contains("timestamp")) {
            ts = m["timestamp"].toString().toLongLong();
        }

        QString roleDisplay;
        if (role == "user")           roleDisplay = "你";
        else if (role == "assistant") roleDisplay = "AI";
        else                          continue;   // 跳过 system 等

        appendMarkdownMessage(roleDisplay, content, ts);
    }

    scrollToBottom();
}

// ---------- 消息显示 ----------

void MainWindow::onUserMessageAdded(const QString &content, qint64 timestamp)
{
    appendLine("你", content, timestamp);
}

void MainWindow::onAiMessageStarted(qint64 timestamp)
{
    appendLine("AI", "", timestamp);
}

void MainWindow::onAiChunkReceived(const QString &chunk)
{
    appendToCurrentLine(chunk);
}

void MainWindow::onAiMessageFinished()
{
    // 流式结束，用 Markdown 重新渲染整个对话区
    onSessionLoaded(
        m_controller->sessionMessages(m_controller->activeSessionId()));
}

void MainWindow::appendMarkdownMessage(const QString &role,
                                       const QString &markdown,
                                       qint64 timestamp)
{
    QTextCursor cursor(m_chatView->document());
    cursor.movePosition(QTextCursor::End);

    // 消息之间插入空块，保持间隔
    if (!m_chatView->document()->isEmpty()) {
        cursor.insertBlock();
    }

    // 角色和时间前缀
    QString timeStr;
    if (timestamp > 0) {
        QDateTime dt = QDateTime::fromSecsSinceEpoch(timestamp);
        QString t = (dt.date() == QDate::currentDate())
                        ? dt.toString("HH:mm")
                        : dt.toString("yyyy-MM-dd HH:mm");
        timeStr = QString(" <span style='color:#999; font-size:11px;'>[%1]</span>")
                      .arg(t);
    }

    cursor.insertHtml(QString("<b>%1</b>%2:").arg(
        role.toHtmlEscaped(), timeStr));

    // 正文
    if (!markdown.isEmpty()) {
        cursor.insertBlock();

        QTextDocument tmp;
        tmp.setMarkdown(markdown);

        QTextDocumentFragment frag(&tmp);
        cursor.insertFragment(frag);
    }
}

void MainWindow::onError(const QString &error)
{
    appendLine("错误", error);
}

void MainWindow::appendLine(const QString &role, const QString &content,
                            qint64 timestamp)
{
    QString timeStr;
    if (timestamp > 0) {
        QString t = QDateTime::fromSecsSinceEpoch(timestamp).toString("yyyy-MM-dd HH:mm");
        timeStr = QString(" <span style='color:#999; font-size:11px;'>[%1]</span>")
                      .arg(t);
    }

    QString safe = content.toHtmlEscaped().replace("\n", "<br>");
    QString html = QString("<p><b>%1</b>%2: %3</p>")
                       .arg(role.toHtmlEscaped(), timeStr, safe);

    m_chatView->append(html);
    scrollToBottom();
}

void MainWindow::appendToCurrentLine(const QString &text)
{
    QTextCursor cursor(m_chatView->document());
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text);
    scrollToBottom();
}

void MainWindow::clearChatView()
{
    m_chatView->clear();
}

void MainWindow::scrollToBottom()
{
    auto *bar = m_chatView->verticalScrollBar();
    bar->setValue(bar->maximum());
}