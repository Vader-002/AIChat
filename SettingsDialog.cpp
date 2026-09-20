#include "SettingsDialog.h"
#include "SettingsManager.h"

#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QLabel>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("设置");
    setMinimumWidth(480);

    m_apiKeyEdit = new QLineEdit(SettingsManager::apiKey(), this);
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);   // 显示为圆点
    m_apiKeyEdit->setPlaceholderText("sk-...");

    m_apiUrlEdit = new QLineEdit(SettingsManager::apiUrl(), this);
    m_apiUrlEdit->setPlaceholderText(
        "例：https://api.deepseek.com/chat/completions");

    m_modelEdit = new QLineEdit(SettingsManager::model(), this);
    m_modelEdit->setPlaceholderText("例：deepseek-flash");

    auto *form = new QFormLayout();
    form->addRow("API Key:", m_apiKeyEdit);
    form->addRow("API URL:", m_apiUrlEdit);
    form->addRow("模型名:",  m_modelEdit);

    auto *tip = new QLabel(
        "提示：修改后立即生效。API Key 目前以明文保存在本机。", this);
    tip->setWordWrap(true);
    tip->setStyleSheet("color: #888; font-size: 11px;");

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted,
            this, &SettingsDialog::onAccepted);
    connect(buttons, &QDialogButtonBox::rejected,
            this, &SettingsDialog::reject);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(form);
    mainLayout->addWidget(tip);
    mainLayout->addWidget(buttons);
}

void SettingsDialog::onAccepted()
{
    SettingsManager::setApiKey(m_apiKeyEdit->text().trimmed());
    SettingsManager::setApiUrl(m_apiUrlEdit->text().trimmed());
    SettingsManager::setModel (m_modelEdit->text().trimmed());
    accept();
}