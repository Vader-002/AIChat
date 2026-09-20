#include "SettingsManager.h"

#include <QSettings>

static const char *kOrg = "MyCompany";
static const char *kApp = "AIChat";

QString SettingsManager::defaultApiUrl()
{
    return "https://api.deepseek.com/chat/completions";
}

QString SettingsManager::defaultModel()
{
    return "deepseek-flash";
}

QString SettingsManager::apiKey()
{
    QSettings s(kOrg, kApp);
    return s.value("api/key").toString();
}

void SettingsManager::setApiKey(const QString &key)
{
    QSettings s(kOrg, kApp);
    s.setValue("api/key", key);
}

QString SettingsManager::apiUrl()
{
    QSettings s(kOrg, kApp);
    return s.value("api/url", defaultApiUrl()).toString();
}

void SettingsManager::setApiUrl(const QString &url)
{
    QSettings s(kOrg, kApp);
    s.setValue("api/url", url);
}

QString SettingsManager::model()
{
    QSettings s(kOrg, kApp);
    return s.value("api/model", defaultModel()).toString();
}

void SettingsManager::setModel(const QString &model)
{
    QSettings s(kOrg, kApp);
    s.setValue("api/model", model);
}