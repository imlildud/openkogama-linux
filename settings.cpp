#include "settings.h"
#include "paths.h"

#include <QSettings>
#include <QUrl>
#include <QVersionNumber>
#include <algorithm>

namespace {

const QList<GameServer> DefaultServers = {
    {QString(), "https://war4.openkogama.org", ">WAR 4< (map by opnfeniks)", {}, {}},
    {QString(), "https://hyrule.openkogama.org", "2 Player Zelda (map by Caninos)", {}, {}},
};

QSettings store()
{
    return QSettings(Paths::data() + "/settings.ini", QSettings::IniFormat);
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
        if (url.path() == "/") url.setPath(QString());
        list.append({settings.value("name").toString(), url.toString(QUrl::StripTrailingSlash),
            settings.value("title").toString(), settings.value("thumbnail").toByteArray(), settings.value("clients").toStringList()});
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
        settings.setValue("clients", servers[i].clients);
    }
    settings.endArray();
}

void addDefaultServers()
{
    const QStringList offered = store().value("launcher/offeredServers").toStringList();
    QStringList added = offered;
    QList<GameServer> list = servers();
    for (const GameServer &server : DefaultServers) {
        if (offered.contains(server.address)) continue;
        added.append(server.address);
        if (std::none_of(list.cbegin(), list.cend(), [&](const GameServer &s) { return s.address == server.address; }))
            list.append(server);
    }
    if (added == offered) return;
    setServers(list);
    store().setValue("launcher/offeredServers", added);
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

QString nickname()
{
    return store().value("player/nickname").toString();
}

void setNickname(const QString &name)
{
    store().setValue("player/nickname", name);
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

bool GameServer::accepts(const QString &version) const
{
    if (clients.isEmpty()) return true;
    const QVersionNumber number = QVersionNumber::fromString(version);
    for (const QString &entry : clients) {
        const QStringList range = entry.split('-');
        const QVersionNumber low = QVersionNumber::fromString(range.value(0).trimmed());
        const QVersionNumber high = QVersionNumber::fromString(range.value(1).trimmed());
        if (range.size() == 2 && !low.isNull() && !high.isNull()) {
            if (!number.isNull() && low <= number && number <= high) return true;
        } else if (entry.trimmed() == version) {
            return true;
        }
    }
    return false;
}
