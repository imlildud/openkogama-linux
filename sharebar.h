#pragma once

#include <QFrame>
#include <QPointer>

class QLabel;
class QPushButton;
class QTimer;
class Tunnel;

class ShareBar : public QFrame
{
    Q_OBJECT

public:
    explicit ShareBar(QWidget *parent = nullptr);
    void share(int world, const QString &name, const QString &version);
    void setVersion(const QString &version);
    void stop();
    int players() const { return m_players; }

private:
    void start();
    void heard(const QString &word, const QString &rest);
    void ended();
    void refresh();

    QLabel *m_text;
    QLabel *m_code;
    QLabel *m_info;
    QPushButton *m_copy;
    QPushButton *m_stop;
    QTimer *m_slow;
    QPointer<Tunnel> m_tunnel;
    QString m_name;
    QString m_version;
    QString m_codeText;
    QString m_error;
    int m_world = 0;
    int m_players = 0;
    bool m_waiting = false;
};
