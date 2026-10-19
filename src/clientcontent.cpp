#include "clientcontent.h"
#include "paths.h"
#include <QCursor>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QUrlQuery>
#include <QtEndian>

static constexpr unsigned long CreateNoWindow = 0x08000000;
static constexpr int ContentTimeout = 3000;
static constexpr int PatchTimeout = 60000;
static constexpr quint32 CompressionMask = 0x3F;

namespace {

QString dataFile(const QString &instancePath) {
    const QDir instance(instancePath);
    for (const QString &folder : instance.entryList({"*_Data"}, QDir::Dirs)) {
        const QString file = instance.filePath(folder + "/data.unity3d");
        if (QFile::exists(file)) return file;
    }
    return {};
}

bool patched(const QString &file) {
    QFile archive(file);
    if (!archive.open(QIODevice::ReadOnly)) return false;
    const QByteArray head = archive.read(128);
    int at = head.indexOf('\0') + 1 + 4;
    at = head.indexOf('\0', at) + 1;
    at = head.indexOf('\0', at) + 1;
    if (at <= 0 || at + 20 > head.size()) return false;
    const quint32 flags = qFromBigEndian<quint32>(head.constData() + at + 16);
    return (flags & CompressionMask) == 0;
}

bool replace(const QString &from, const QString &to) {
    const QString temporary = to + ".tmp";
    QFile::remove(temporary);
    if (!QFile::copy(from, temporary)) return false;
    if (QFile::exists(to) && !QFile::remove(to)) {
        QFile::remove(temporary);
        return false;
    }
    return QFile::rename(temporary, to);
}

QJsonArray sounds(const QString &serverUrl, const QString &version) {
    QUrl url(serverUrl + "/api/content");
    QUrlQuery query;
    query.addQueryItem("client", version);
    url.setQuery(query);
    QNetworkRequest request(url);
    request.setTransferTimeout(ContentTimeout);

    QNetworkAccessManager net;
    QNetworkReply *reply = net.get(request);
    QEventLoop wait;
    QObject::connect(reply, &QNetworkReply::finished, &wait, &QEventLoop::quit);
    wait.exec(QEventLoop::ExcludeUserInputEvents);
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) return {};
    return QJsonDocument::fromJson(reply->readAll()).object().value("sounds").toArray();
}

bool patch(const QString &original, const QJsonArray &list, const QString &data) {
    QProcess process;
    process.setProgram(QDir(Paths::server()).filePath("openkogama-server.exe"));
    process.setArguments({"client", "patch", original, "-", data});
    process.setWorkingDirectory(Paths::serverData());
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args) {
        args->flags |= CreateNoWindow;
    });
    process.start();
    process.write(QJsonDocument(QJsonObject{{"sounds", list}}).toJson(QJsonDocument::Compact));
    process.closeWriteChannel();
    const bool done = process.waitForFinished(PatchTimeout) && process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
    if (!done) qWarning("Client patch failed: %s", process.readAll().constData());
    return done;
}

}

void ClientContent::prepare(const QString &instancePath, const QString &serverUrl, const QString &version) {
    const QString data = dataFile(instancePath);
    if (data.isEmpty()) return;

    const QString original = data + ".original";
    if (!patched(data) && QFileInfo(original).size() != QFileInfo(data).size()) {
        QFile::remove(original);
        if (!QFile::copy(data, original)) return;
    }
    if (!QFile::exists(original)) return;

    const QJsonArray list = sounds(serverUrl, version);
    if (list.isEmpty()) {
        if (patched(data)) replace(original, data);
        return;
    }

    QGuiApplication::setOverrideCursor(Qt::WaitCursor);
    if (!patch(original, list, data) && patched(data)) replace(original, data);
    QGuiApplication::restoreOverrideCursor();
}
