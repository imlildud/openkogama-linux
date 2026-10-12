#include "serverspage.h"
#include "settings.h"
#include "gamelauncher.h"
#include "centereddelegate.h"
#include <QApplication>
#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QListWidget>
#include <QPushButton>
#include <QShortcut>
#include <QSignalSpy>
#include <QPointer>
#include <QProcess>
#include <QStandardPaths>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>
#include <QTemporaryDir>

class MockServer : public QTcpServer
{
public:
    QByteArray metadata = "{\"title\":\"A friend's server\",\"thumbnail\":\"/api/server/thumbnail\"}";
    QByteArray image;
    int metadataStatus = 200;
    int imageStatus = 200;
    QStringList requests;

    MockServer()
    {
        listen(QHostAddress::LocalHost);
        QImage tile(8, 8, QImage::Format_RGB32);
        tile.fill(Qt::red);
        QBuffer buffer(&image);
        buffer.open(QIODevice::WriteOnly);
        tile.save(&buffer, "PNG");
        connect(this, &QTcpServer::newConnection, this, [this]() {
            while (hasPendingConnections()) {
                auto *socket = nextPendingConnection();
                connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
                connect(socket, &QTcpSocket::readyRead, this, [this, socket, data = QByteArray()]() mutable {
                    data += socket->readAll();
                    if (!data.contains("\r\n\r\n")) return;
                    const QString path = QString::fromLatin1(data.split(' ').value(1));
                    requests.append(path);
                    int status = path == "/api/server" ? metadataStatus : path == "/api/server/thumbnail" ? imageStatus : 404;
                    QByteArray body = path == "/api/server" ? metadata : path == "/api/server/thumbnail" ? image : QByteArray();
                    if (path == "/ping") { status = 200; body = "{}"; }
                    socket->write("HTTP/1.1 " + QByteArray::number(status) + " Test\r\nContent-Length: "
                        + QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" + body);
                    socket->disconnectFromHost();
                    data.clear();
                });
            }
        });
    }

    QString url() const { return "http://127.0.0.1:" + QString::number(serverPort()); }
};

static QString tile(const QListWidgetItem *item)
{
    return item->text() + "\n" + item->data(CenteredDelegate::SubtitleRole).toString();
}

class ServersPageTest : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QApplication::setOrganizationName("OpenKogamaTests");
        QApplication::setApplicationName("Launcher-" + QString::number(QApplication::applicationPid()));
    }

    void cleanup() { Settings::setServers({}); Settings::setCurrentServer({}); }

    void metadataJoinAndOfflineCache()
    {
        MockServer server;
        QVERIFY(server.isListening());
        Settings::setServers({{"My friend", server.url(), {}, {}}});
        QByteArray cached;
        {
            ServersPage page;
            auto *list = page.findChild<QListWidget *>("serversList");
            QVERIFY(list);
            QTRY_VERIFY(tile(list->item(0)).contains("Online"));
            QTRY_VERIFY(!Settings::servers().first().thumbnail.isEmpty());
            QCOMPARE(tile(list->item(0)), QString("A friend's server\nOnline"));
            QCOMPARE(server.requests, QStringList({"/api/server", "/api/server/thumbnail"}));
            cached = Settings::servers().first().thumbnail;
            QCOMPARE(QImage::fromData(cached).size(), QSize(400, 160));
            QSignalSpy joined(&page, &ServersPage::joinRequested);
            auto *join = page.findChild<QPushButton *>("joinServerButton");
            QVERIFY(!join->isEnabled());
            page.setCanJoin(true);
            join->click();
            QCOMPARE(joined.size(), 1);
            QCOMPARE(joined.first().first().toString(), server.url());
            server.close();
            emit page.findChild<QShortcut *>("refreshServersShortcut")->activated();
            QTRY_COMPARE(tile(list->item(0)), QString("A friend's server\nOffline"));
            QCOMPARE(Settings::servers().first().thumbnail, cached);
        }
        ServersPage reopened;
        auto *list = reopened.findChild<QListWidget *>("serversList");
        QTRY_COMPARE(tile(list->item(0)), QString("A friend's server\nOffline"));
        QCOMPARE(Settings::servers().first().thumbnail, cached);
    }

    void rejectsOversizedImageAndTruncatesTitle()
    {
        MockServer server;
        server.metadata = QJsonDocument(QJsonObject{{"title", QString(80, 'A')},
            {"thumbnail", "/api/server/thumbnail"}, {"worlds", "ignored"}}).toJson();
        server.image = QByteArray(256 * 1024 + 1, 'X');
        Settings::setServers({{"Saved", server.url(), {}, {}}});
        ServersPage page;
        auto *list = page.findChild<QListWidget *>("serversList");
        QTRY_COMPARE(Settings::servers().first().title.size(), 40);
        QTRY_VERIFY(server.requests.contains("/api/server/thumbnail"));
        QTest::qWait(100);
        QVERIFY(Settings::servers().first().thumbnail.isEmpty());
        QCOMPARE(tile(list->item(0)), QString(40, 'A') + "\nOnline");
        QCOMPARE(server.requests.size(), 2);
    }

    void rejectsInvalidImagesAndExternalThumbnailURLs()
    {
        MockServer server;
        server.image = "not an image";
        Settings::setServers({{"Saved", server.url(), {}, {}}});
        ServersPage page;
        QTRY_VERIFY(server.requests.contains("/api/server/thumbnail"));
        QTest::qWait(100);
        QVERIFY(Settings::servers().first().thumbnail.isEmpty());
        server.requests.clear();
        server.metadata = "{\"title\":\"Safe\",\"thumbnail\":\"https://example.com/image.png\"}";
        emit page.findChild<QShortcut *>("refreshServersShortcut")->activated();
        QTRY_COMPARE(Settings::servers().first().title, QString("Safe"));
        QCOMPARE(server.requests, QStringList({"/api/server"}));
    }

    void oldServerUsesOnlyPingFallback()
    {
        MockServer server;
        server.metadataStatus = 404;
        Settings::setServers({{"Old server", server.url(), {}, {}}});
        ServersPage page;
        auto *list = page.findChild<QListWidget *>("serversList");
        QTRY_COMPARE(tile(list->item(0)), QString("Old server\nOnline"));
        QCOMPARE(server.requests, QStringList({"/api/server", "/ping"}));
    }

    void preservesUnicodeTitles()
    {
        MockServer server;
        server.metadata = QJsonDocument(QJsonObject{{"title", QString::fromUtf8("😀").repeated(45)}}).toJson();
        Settings::setServers({{"Saved", server.url(), {}, {}}});
        ServersPage page;
        QTRY_COMPARE(Settings::servers().first().title.toUcs4().size(), 40);
    }

    void joinStartsGameWithServerSession()
    {
        QTemporaryDir instance;
        QVERIFY(instance.isValid());
        QFile meta(instance.filePath("instance.json"));
        QVERIFY(meta.open(QIODevice::WriteOnly));
        meta.write("{\"name\":\"Test\",\"version\":\"2015\",\"unityVersion\":\"5.2\"}");
        meta.close();
        QWidget owner;
        QPointer<QProcess> client = GameLauncher::start(instance.path(), QApplication::applicationFilePath(),
            "http://127.0.0.1:8080", "play", 0, &owner, &owner);
        QVERIFY(client);
        const QString capture = instance.filePath("launch-capture.json");
        QTRY_VERIFY(QFile::exists(capture));
        QFile result(capture);
        QVERIFY(result.open(QIODevice::ReadOnly));
        const QJsonObject launched = QJsonDocument::fromJson(result.readAll()).object();
        QCOMPARE(launched.value("session").toString(), QString("http://127.0.0.1:8080/session?mode=play&client=2015&unity=5.2"));
        QCOMPARE(launched.value("cwd").toString(), instance.path());
        QTRY_VERIFY(!client || client->state() == QProcess::NotRunning);
    }

    void cleanupTestCase()
    {
        const QString settings = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/settings.ini";
        QFile::remove(settings);
        QDir().rmdir(QFileInfo(settings).absolutePath());
    }
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    const QString package = app.arguments().value(1);
    if (package.startsWith("kogamaPackage:")) {
        QFile capture(QDir::current().filePath("launch-capture.json"));
        if (!capture.open(QIODevice::WriteOnly)) return 1;
        capture.write(QJsonDocument(QJsonObject{{"session", QString::fromUtf8(QByteArray::fromBase64(package.mid(14).toLatin1()))},
            {"cwd", QDir::currentPath()}}).toJson());
        return 0;
    }
    ServersPageTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "serverspage_test.moc"
