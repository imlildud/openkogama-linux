#ifndef WORLDSDIALOG_H
#define WORLDSDIALOG_H

#include <QDialog>
#include <QIcon>
#include <QListWidgetItem>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QProcess>
#include <functional>

namespace Ui {
class WorldsDialog;
}

class WorldsDialog : public QDialog
{
    Q_OBJECT

public:
    WorldsDialog(const QString &path, const QString &exe, QProcess *server, QWidget *parent = nullptr);
    ~WorldsDialog();
    void setInstance(const QString &path, const QString &exe, QProcess *server = nullptr);
    bool hasRunningGames() const;
    QWidget *worldsPage() const;
    QWidget *avatarsPage() const;
    void showAvatars();
    void reject() override;

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void loadWorlds(int select = 0);
    void filterWorlds();
    void updateWorldActions();
    void updateAvatarActions();
    void loadTemplates();
    void loadThumbnail(QListWidgetItem *item, int world);
    void checkRevision();
    void createWorld();
    void importWorld();
    void exportWorld();
    void renameWorld();
    void deleteWorld();
    void onWorldRenamed(QListWidgetItem *item);
    void showWorldMenu(const QPoint &pos);
    void launch(const QString &mode, int world);
    int selectedWorld() const;
    void loadAvatars(int select = 0);
    void loadAvatarPicture(QListWidgetItem *item, int avatar, const QString &version);
    void useAvatar();
    void editAvatar();
    void activateAvatar(int avatar, std::function<void()> done);
    void importAvatar();
    void exportAvatar();
    void renameAvatar();
    void onAvatarRenamed(QListWidgetItem *item);
    void showAvatarMenu(const QPoint &pos);
    int selectedAvatar() const;

    Ui::WorldsDialog *ui;
    QPointer<QWidget> m_owner;
    QString m_path;
    QString m_title;
    QString m_exe;
    const QString m_url = "http://127.0.0.1:8080";
    QPointer<QProcess> m_server;
    QNetworkAccessManager *m_net;
    QList<QPair<QString, QString>> m_templates;
    int m_revision = -1;
    bool m_hasAvatarEditor = true;
    QList<QPointer<QProcess>> m_clients;
};

#endif // WORLDSDIALOG_H
