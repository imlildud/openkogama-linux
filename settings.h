#pragma once

#include <QList>
#include <QByteArray>
#include <QString>
#include <QStringList>

struct GameServer
{
    QString name;
    QString address;
    QString title;
    QByteArray thumbnail;
    QStringList clients;

    bool accepts(const QString &version) const;
};

namespace Settings {

QList<GameServer> servers();
void setServers(const QList<GameServer> &servers);
void addDefaultServers();
QString currentServer();
void setCurrentServer(const QString &address);

QString lastInstance();
void setLastInstance(const QString &path);

QString nickname();
void setNickname(const QString &name);

bool consoleOnLaunch();
bool consoleOnCrash();
void setConsoleOnLaunch(bool enabled);
void setConsoleOnCrash(bool enabled);

}
