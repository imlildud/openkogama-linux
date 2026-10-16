#pragma once

#include <QDialog>
#include <QJsonObject>

class QLabel;
class QPlainTextEdit;

class ServerConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ServerConfigDialog(QWidget *parent = nullptr);
    static QJsonObject current();

private:
    void save();

    QPlainTextEdit *m_text;
    QLabel *m_error;
};
