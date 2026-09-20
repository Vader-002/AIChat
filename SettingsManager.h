#pragma once

#include <QString>

class SettingsManager
{
public:
    static QString apiKey();
    static void setApiKey(const QString &key);

    static QString apiUrl();
    static void setApiUrl(const QString &url);

    static QString model();
    static void setModel(const QString &model);

    // 默认值
    static QString defaultApiUrl();
    static QString defaultModel();
};