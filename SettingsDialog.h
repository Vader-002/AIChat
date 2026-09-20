#pragma once

#include <QDialog>

class QLineEdit;

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);

private slots:
    void onAccepted();

private:
    QLineEdit *m_apiKeyEdit;
    QLineEdit *m_apiUrlEdit;
    QLineEdit *m_modelEdit;
};