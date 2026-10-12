#include "settings.h"

#include <QSettings>
#include <QStandardPaths>
#include <QUrl>

namespace {

QSettings store()
{
    return QSettings(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/settings.ini", QSettings::IniFormat);
}

}

namespace Settings {

QList<GameServer> servers()
{
    QSettings settings = store();
    QList<GameServer> list;
    int count = settings.beginReadArray("servers");
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        const QString address = settings.value("address").toString().trimmed();
        QUrl url(address.contains("://") ? address : "http://" + address);
        if (!url.isValid() || url.host().isEmpty() || (url.scheme() != "http" && url.scheme() != "https")
            || !url.userInfo().isEmpty() || url.hasQuery() || url.hasFragment() || url.port() == 0
            || (!url.path().isEmpty() && url.path() != "/")) continue;
        if (!address.contains("://") && url.port() == -1) url.setPort(8080);
        list.append({settings.value("name").toString(), url.toString(QUrl::StripTrailingSlash),
            settings.value("title").toString(), settings.value("thumbnail").toByteArray()});
    }
    settings.endArray();
    return list;
}

void setServers(const QList<GameServer> &servers)
{
    QSettings settings = store();
    settings.remove("servers");
    settings.beginWriteArray("servers", servers.size());
    for (int i = 0; i < servers.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("name", servers[i].name);
        settings.setValue("address", servers[i].address);
        settings.setValue("title", servers[i].title);
        settings.setValue("thumbnail", servers[i].thumbnail);
    }
    settings.endArray();
}

QString currentServer()
{
    return store().value("launcher/server").toString();
}

void setCurrentServer(const QString &address)
{
    store().setValue("launcher/server", address);
}

QString lastInstance()
{
    return store().value("launcher/lastInstance").toString();
}

void setLastInstance(const QString &path)
{
    store().setValue("launcher/lastInstance", path);
}

bool consoleOnLaunch()
{
    return store().value("console/showOnLaunch", false).toBool();
}

bool consoleOnCrash()
{
    return store().value("console/showOnCrash", true).toBool();
}

void setConsoleOnLaunch(bool enabled)
{
    store().setValue("console/showOnLaunch", enabled);
}

void setConsoleOnCrash(bool enabled)
{
    store().setValue("console/showOnCrash", enabled);
}

}
