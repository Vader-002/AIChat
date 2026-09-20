#pragma once

#include <QString>
#include <QJsonObject>
#include <QList>

struct Session {
    QString id;
    QString title;
    QString systemPrompt;
    QList<QJsonObject> messages;
};