#pragma once

#include <QList>
#include <QByteArray>
#include <QString>

struct GameServer
{
    QString name;
    QString address;
    QString title;
    QByteArray thumbnail;
};

namespace Settings {

QList<GameServer> servers();
void setServers(const QList<GameServer> &servers);
QString currentServer();
void setCurrentServer(const QString &address);

QString lastInstance();
void setLastInstance(const QString &path);

bool consoleOnLaunch();
bool consoleOnCrash();
void setConsoleOnLaunch(bool enabled);
void setConsoleOnCrash(bool enabled);

}
