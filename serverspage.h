#pragma once

#include <QWidget>
#include <QSet>
#include <functional>

class QEvent;
class QJsonObject;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QNetworkAccessManager;
class QPushButton;
class QToolButton;

class ServersPage : public QWidget
{
    Q_OBJECT

public:
    explicit ServersPage(QWidget *parent = nullptr);
    void setCanJoin(bool enabled);
    void showFound(const QString &address, const QJsonObject &info);
    void probeThrough(const QString &address, const QString &base);

signals:
    void joinRequested(const QString &address);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void load(const QString &select = {});
    void refresh();
    void probe(const QString &address);
    void probeCode(const QString &address);
    void get(const QString &url, qint64 limit, std::function<void(int, const QByteArray &)> done);
    void resolve(QStringList candidates, std::function<void(const QString &)> done);
    void keepServer(const QString &address, const QString &name);
    void fetchThumbnail(const QString &address, const QString &base);
    void setStatus(const QString &address, const QString &status);
    void updateActions();
    void filter();
    void addServer();
    void joinWithCode();
    void removeServer();
    void copyAddress();
    void showMenu(const QPoint &pos);
    void join();
    QListWidgetItem *find(const QString &address) const;
    QString selectedAddress() const;

    QNetworkAccessManager *m_net;
    QListWidget *m_list;
    QLineEdit *m_search;
    QLabel *m_empty;
    QLabel *m_selected;
    QLabel *m_address;
    QToolButton *m_more;
    QPushButton *m_join;
    QPushButton *m_joinCode;
    QSet<QString> m_pending;
    bool m_canJoin = false;
};
