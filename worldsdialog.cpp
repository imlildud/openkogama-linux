#include "worldsdialog.h"
#include "ui_worldsdialog.h"
#include "launchdialog.h"
#include "gamelauncher.h"
#include "centereddelegate.h"
#include "tilegrid.h"
#include "webplayerruntime.h"
#include "consolewindow.h"
#include "settings.h"
#include <QEvent>
#include <QKeyEvent>
#include <QStyle>
#include <QPixmap>
#include <QPainter>
#include <QTimer>
#include <algorithm>

static constexpr int NameRole = Qt::UserRole + 1;
static constexpr int ImageRole = Qt::UserRole + 2;
static constexpr int ActiveRole = Qt::UserRole + 3;
static const QUrl MapsArchive("https://cdn.openkogama.org/maps/v1/");

#include <QBuffer>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QIcon>
#include <QImage>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QInputDialog>
#include <QMenu>
#include <QShortcut>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkReply>
#include <QPointer>
#include <QRegularExpression>
#include <QUrlQuery>

static QString instanceTitle(const QString &path)
{
    QFile file(QDir(path).filePath("instance.json"));
    QJsonObject info;
    if (file.open(QIODevice::ReadOnly))
        info = QJsonDocument::fromJson(file.readAll()).object();
    for (const char *key : {"name", "version"}) {
        QString value = info.value(key).toString().trimmed();
        if (!value.isEmpty())
            return "KoGaMa " + value;
    }
    return "KoGaMa";
}

WorldsDialog::WorldsDialog(const QString &path, const QString &exe, QProcess *server, QWidget *parent)
    : QDialog(nullptr)
    , ui(new Ui::WorldsDialog)
    , m_owner(parent)
    , m_path(path)
    , m_title(instanceTitle(path))
    , m_exe(exe)
    , m_server(server)
    , m_net(new QNetworkAccessManager(this))
{
    ui->setupUi(this);
    m_net->setTransferTimeout(8000);
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    setWindowIcon(QIcon(":/launch.png"));
    setWindowTitle(m_title);

    connect(ui->playButton, &QPushButton::clicked, this, [this]() { launch("play", selectedWorld()); });
    connect(ui->buildButton, &QPushButton::clicked, this, [this]() { launch("edit", selectedWorld()); });
    connect(ui->hostButton, &QPushButton::clicked, this, [this]() {
        const int world = selectedWorld();
        if (world == 0) return;
        emit hostRequested(world, ui->worldsList->currentItem()->data(NameRole).toString());
        launch("play", world);
    });
    connect(ui->editAvatarButton, &QPushButton::clicked, this, &WorldsDialog::editAvatar);
    connect(ui->newButton, &QPushButton::clicked, this, &WorldsDialog::createWorld);
    connect(ui->importButton, &QPushButton::clicked, this, &WorldsDialog::importWorld);
    auto *worldMenu = new QMenu(this);
    worldMenu->addAction("Export...", this, &WorldsDialog::exportWorld);
    worldMenu->addAction("Rename", this, &WorldsDialog::renameWorld);
    worldMenu->addSeparator();
    worldMenu->addAction("Delete...", this, &WorldsDialog::deleteWorld);
    ui->moreWorldButton->setMenu(worldMenu);
    auto *renameShortcut = new QShortcut(QKeySequence(Qt::Key_F2), ui->worldsList);
    renameShortcut->setContext(Qt::WidgetShortcut);
    connect(renameShortcut, &QShortcut::activated, this, &WorldsDialog::renameWorld);
    connect(ui->worldsList, &QListWidget::itemChanged, this, &WorldsDialog::onWorldRenamed);
    auto *deleteShortcut = new QShortcut(QKeySequence::Delete, ui->worldsList);
    deleteShortcut->setContext(Qt::WidgetShortcut);
    connect(deleteShortcut, &QShortcut::activated, this, &WorldsDialog::deleteWorld);
    connect(ui->worldsList, &QListWidget::customContextMenuRequested, this, &WorldsDialog::showWorldMenu);
    auto *refreshWorlds = new QShortcut(QKeySequence::Refresh, ui->worldsTab);
    refreshWorlds->setContext(Qt::WidgetWithChildrenShortcut);
    connect(refreshWorlds, &QShortcut::activated, this, [this]() { loadWorlds(selectedWorld()); });
    auto *refreshAvatars = new QShortcut(QKeySequence::Refresh, ui->avatarsTab);
    refreshAvatars->setContext(Qt::WidgetWithChildrenShortcut);
    connect(refreshAvatars, &QShortcut::activated, this, [this]() { loadAvatars(selectedAvatar()); });
    connect(ui->worldSearchLineEdit, &QLineEdit::textChanged, this, [this]() { filterWorlds(); });
    connect(ui->worldsList, &QListWidget::currentItemChanged, this, [this]() { updateWorldActions(); });
    connect(ui->avatarsList, &QListWidget::currentItemChanged, this, [this]() { updateAvatarActions(); });
    connect(ui->worldsList, &QListWidget::itemActivated, this, [this]() { launch("play", selectedWorld()); });

    connect(ui->useAvatarButton, &QPushButton::clicked, this, &WorldsDialog::useAvatar);
    connect(ui->importAvatarButton, &QPushButton::clicked, this, &WorldsDialog::importAvatar);
    auto *avatarMenu = new QMenu(this);
    avatarMenu->addAction("Rename", this, &WorldsDialog::renameAvatar);
    avatarMenu->addAction("Export...", this, &WorldsDialog::exportAvatar);
    ui->moreAvatarButton->setMenu(avatarMenu);
    connect(ui->avatarsList, &QListWidget::itemChanged, this, &WorldsDialog::onAvatarRenamed);
    auto *renameAvatarShortcut = new QShortcut(QKeySequence(Qt::Key_F2), ui->avatarsList);
    renameAvatarShortcut->setContext(Qt::WidgetShortcut);
    connect(renameAvatarShortcut, &QShortcut::activated, this, &WorldsDialog::renameAvatar);
    connect(ui->avatarsList, &QListWidget::itemActivated, this, &WorldsDialog::useAvatar);
    connect(ui->avatarsList, &QListWidget::customContextMenuRequested, this, &WorldsDialog::showAvatarMenu);

    TileGrid::setup(ui->worldsList, this);
    TileGrid::setup(ui->avatarsList, this);
    static_cast<CenteredDelegate *>(ui->avatarsList->itemDelegate())->setBadgeRole(ActiveRole);

    setInstance(path, exe);
    updateWorldActions();
    updateAvatarActions();
    loadWorlds();
    loadTemplates();
    loadAvatars();

    auto *poll = new QTimer(this);
    connect(poll, &QTimer::timeout, this, &WorldsDialog::checkRevision);
    poll->start(1000);
}

WorldsDialog::~WorldsDialog()
{
    delete ui;
}

void WorldsDialog::setInstance(const QString &path, const QString &exe, QProcess *server) {
    if (server) m_server = server;
    m_path = path;
    m_exe = exe;
    m_title = instanceTitle(path);
    QFile file(QDir(path).filePath("instance.json"));
    QJsonObject info;
    if (file.open(QIODevice::ReadOnly))
        info = QJsonDocument::fromJson(file.readAll()).object();
    m_hasAvatarEditor = !WebPlayerRuntime::isWebPlayerFile(exe)
        || !info.value("unityVersion").toString().startsWith("3.4");
    ui->editAvatarButton->setToolTip(m_hasAvatarEditor ? QString() : "This game version has no avatar editor");
    updateAvatarActions();
}

QWidget *WorldsDialog::worldsPage() const {
    return ui->worldsTab;
}

QWidget *WorldsDialog::avatarsPage() const {
    return ui->avatarsTab;
}

void WorldsDialog::showAvatars() {
    loadAvatars(selectedAvatar());
}

bool WorldsDialog::hasRunningGames() const {
    return std::any_of(m_clients.cbegin(), m_clients.cend(), [](const QPointer<QProcess> &client) {
        return client && client->state() != QProcess::NotRunning;
    });
}

void WorldsDialog::updateWorldActions() {
    QListWidgetItem *item = ui->worldsList->currentItem();
    bool selected = item && !item->isHidden();
    ui->playButton->setEnabled(selected);
    ui->hostButton->setEnabled(selected);
    ui->buildButton->setEnabled(selected);
    ui->moreWorldButton->setEnabled(selected);
    ui->selectedWorldLabel->setText(selected ? item->text() : "No world selected");
}

void WorldsDialog::filterWorlds() {
    const QString search = ui->worldSearchLineEdit->text().trimmed();
    QListWidgetItem *first = nullptr;
    int visible = 0;
    for (int i = 0; i < ui->worldsList->count(); ++i) {
        auto *item = ui->worldsList->item(i);
        bool matches = item->text().contains(search, Qt::CaseInsensitive);
        item->setHidden(!matches);
        if (matches) {
            ++visible;
            if (!first) first = item;
        }
    }
    auto *current = ui->worldsList->currentItem();
    if (!current || current->isHidden())
        ui->worldsList->setCurrentItem(first);
    int total = ui->worldsList->count();
    ui->worldSearchLineEdit->setPlaceholderText(total > 0 ? "Search " + QString::number(total) + (total == 1 ? " world..." : " worlds...") : "Search your worlds...");
    ui->worldStatusLabel->setText(total == 0
        ? "No worlds yet. Click New world to create one, or Import to open a saved file."
        : visible == 0 ? "No worlds match your search." : QString());
    ui->worldStatusLabel->setVisible(!ui->worldStatusLabel->text().isEmpty());
    updateWorldActions();
}

void WorldsDialog::updateAvatarActions() {
    QListWidgetItem *item = ui->avatarsList->currentItem();
    const bool active = item && item->data(ActiveRole).toBool();
    ui->useAvatarButton->setEnabled(item && !active);
    ui->editAvatarButton->setEnabled(m_hasAvatarEditor);
    ui->moreAvatarButton->setEnabled(item != nullptr);
    ui->selectedAvatarLabel->setText(item ? item->text() : "No avatar selected");
}

void WorldsDialog::keyPressEvent(QKeyEvent *event) {
    if (isWindow())
        QDialog::keyPressEvent(event);
    else
        QWidget::keyPressEvent(event);
}

void WorldsDialog::reject() {
    if (m_server)
        LaunchDialog::stopServer(m_server);
    QDialog::reject();
}

void WorldsDialog::loadWorlds(int select) {
    if (ui->worldsList->count() == 0) {
        ui->worldStatusLabel->setText("Loading your worlds...");
        ui->worldStatusLabel->show();
    }
    QNetworkReply *reply = m_net->get(QNetworkRequest(QUrl(m_url + "/api/worlds")));
    connect(reply, &QNetworkReply::finished, this, [this, reply, select, url = m_url]() {
        reply->deleteLater();
        if (url != m_url) return;
        if (reply->error() != QNetworkReply::NoError) {
            ui->worldStatusLabel->setText("Could not load your worlds. Press F5 to try again.");
            ui->worldStatusLabel->show();
            return;
        }
        const QJsonArray worlds = QJsonDocument::fromJson(reply->readAll()).array();
        QSignalBlocker blocker(ui->worldsList);

        QList<int> ids;
        for (const QJsonValue &value : worlds)
            ids.append(value.toObject()["id"].toInt());
        QList<int> shown;
        for (int i = 0; i < ui->worldsList->count(); ++i)
            shown.append(ui->worldsList->item(i)->data(Qt::UserRole).toInt());
        if (ids != shown) {
            ui->worldsList->clear();
            const QIcon empty = TileGrid::icon(TileGrid::placeholder(TileGrid::WideImage, palette().color(QPalette::Button)));
            for (int id : std::as_const(ids)) {
                auto *item = new QListWidgetItem(empty, QString(), ui->worldsList);
                item->setFlags(item->flags() | Qt::ItemIsEditable);
                item->setData(Qt::UserRole, id);
            }
        }

        auto date = [](const QJsonValue &value) {
            return QDateTime::fromString(value.toString(), Qt::ISODateWithMs).toLocalTime().toString("yyyy-MM-dd HH:mm");
        };
        for (int i = 0; i < worlds.size(); ++i) {
            QJsonObject world = worlds[i].toObject();
            QListWidgetItem *item = ui->worldsList->item(i);
            QString name = world["name"].toString();
            if (item->data(NameRole).toString() != name) {
                item->setText(name);
                item->setData(NameRole, name);
            }
            QString tip = "Last played: " + (world["playedAt"].isString() ? date(world["playedAt"]) : QString("never")) + "\n"
                + "Saved: " + date(world["savedAt"]) + "\n"
                + (world["publishedAt"].isString() ? "Published: " + date(world["publishedAt"]) : QString("Not published"));
            if (item->toolTip() != tip)
                item->setToolTip(tip);
            if (ids[i] == select)
                ui->worldsList->setCurrentItem(item);
            loadThumbnail(item, ids[i]);
        }
        if (!ui->worldsList->currentItem() && ui->worldsList->count() > 0)
            ui->worldsList->setCurrentRow(0);
        filterWorlds();
    });
}

void WorldsDialog::checkRevision() {
    QNetworkRequest request(QUrl(m_url + "/api/worlds/revision"));
    request.setTransferTimeout(500);
    QNetworkReply *reply = m_net->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) return;

        int revision = QJsonDocument::fromJson(reply->readAll()).object()["revision"].toInt();
        if (revision == m_revision) return;
        bool first = m_revision < 0;
        m_revision = revision;
        if (!first && !ui->worldsList->viewport()->findChild<QLineEdit *>())
            loadWorlds(selectedWorld());
    });
}

void WorldsDialog::loadThumbnail(QListWidgetItem *item, int world) {
    QPointer<QListWidget> list = ui->worldsList;
    QNetworkReply *reply = m_net->get(QNetworkRequest(QUrl(m_url + "/images/0/" + QString::number(world) + ".png?v=" + QString::number(m_revision))));
    connect(reply, &QNetworkReply::finished, this, [reply, list, item, world]() {
        reply->deleteLater();
        QByteArray data = reply->readAll();
        QPixmap image;
        if (reply->error() != QNetworkReply::NoError || !list) return;
        for (int i = 0; i < list->count(); ++i) {
            if (list->item(i) != item || item->data(Qt::UserRole).toInt() != world || item->data(ImageRole).toByteArray() == data) continue;
            if (!image.loadFromData(data)) return;
            item->setData(ImageRole, data);
            item->setIcon(TileGrid::icon(TileGrid::cover(image, TileGrid::WideImage)));
        }
    });
}

bool WorldsDialog::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::Resize && (watched == ui->worldsList || watched == ui->avatarsList)) {
        auto *list = static_cast<QListWidget *>(watched);
        TileGrid::fit(list, list == ui->worldsList ? TileGrid::WideTile : TileGrid::SquareTile);
    }
    return QDialog::eventFilter(watched, event);
}

void WorldsDialog::loadTemplates() {
    QNetworkReply *reply = m_net->get(QNetworkRequest(QUrl(m_url + "/api/templates")));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        m_templates.clear();
        const QJsonArray templates = QJsonDocument::fromJson(reply->readAll()).array();
        for (const QJsonValue &value : templates)
            m_templates.append({value.toObject()["id"].toString(), value.toObject()["name"].toString()});
    });
}

void WorldsDialog::createWorld() {
    QDialog dialog(this);
    dialog.setWindowTitle("New world");

    dialog.setMinimumWidth(380);

    auto *name = new QLineEdit("New world", &dialog);
    name->selectAll();
    auto *templates = new QComboBox(&dialog);
    for (const auto &[id, title] : std::as_const(m_templates))
        templates->addItem(title, id);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    auto *create = buttons->button(QDialogButtonBox::Ok);
    create->setText("Create");
    create->setProperty("primary", true);
    create->style()->polish(create);
    connect(name, &QLineEdit::textChanged, &dialog, [create](const QString &value) { create->setEnabled(!value.trimmed().isEmpty()); });
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);
    auto *form = new QFormLayout;
    form->setSpacing(8);
    form->addRow("Name:", name);
    form->addRow("Template:", templates);
    layout->addLayout(form);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted || name->text().trimmed().isEmpty()) return;

    QUrl url(m_url + "/api/worlds");
    QUrlQuery query;
    query.addQueryItem("name", name->text().trimmed());
    if (templates->currentIndex() >= 0)
        query.addQueryItem("template", templates->currentData().toString());
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
    QNetworkReply *reply = m_net->post(request, QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        loadWorlds(QJsonDocument::fromJson(reply->readAll()).object()["id"].toInt());
    });
}

void WorldsDialog::importWorld() {
    QString path = QFileDialog::getOpenFileName(this, "Import World", QString(), "KoGaMa maps (*.kgmap *.kgm)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Import", "Could not read " + path);
        return;
    }

    QUrl url(m_url + "/api/worlds/import");
    QUrlQuery query;
    query.addQueryItem("name", QFileInfo(path).completeBaseName());
    QFile meta(QDir(m_path).filePath("instance.json"));
    QString version = meta.open(QIODevice::ReadOnly) ? QJsonDocument::fromJson(meta.readAll()).object().value("version").toString() : QString();
    if (!version.isEmpty())
        query.addQueryItem("client", version);
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    const QByteArray data = file.readAll();
    QNetworkReply *reply = m_net->post(request, data);
    connect(reply, &QNetworkReply::finished, this, [this, reply, data]() {
        reply->deleteLater();
        int id = QJsonDocument::fromJson(reply->readAll()).object()["id"].toInt();
        if (id == 0) QMessageBox::warning(this, "Import", "This map could not be imported");
        else findArchiveThumbnail(id, data);
        loadWorlds(id);
    });
}

void WorldsDialog::findArchiveThumbnail(int world, const QByteArray &file) {
    const QString sha = QCryptographicHash::hash(file, QCryptographicHash::Sha256).toHex();
    QNetworkReply *reply = m_net->get(QNetworkRequest(MapsArchive.resolved(QUrl("latest.json"))));
    connect(reply, &QNetworkReply::finished, this, [this, reply, world, sha]() {
        reply->deleteLater();
        const QJsonObject latest = QJsonDocument::fromJson(reply->readAll()).object();
        const QJsonObject hashes = latest["hashes"].toObject();
        if (reply->error() != QNetworkReply::NoError || hashes.isEmpty()) return;
        const QUrl build = MapsArchive.resolved(QUrl("builds/" + latest["build"].toString() + "/"));
        const QUrl images = build.resolved(QUrl(latest["images"].toObject()["path"].toString()));
        QNetworkReply *lookup = m_net->get(QNetworkRequest(build.resolved(QUrl(hashes["path"].toString() + sha.left(hashes["prefix"].toInt(2)) + ".json"))));
        connect(lookup, &QNetworkReply::finished, this, [this, lookup, world, sha, images]() {
            lookup->deleteLater();
            const QString image = QJsonDocument::fromJson(lookup->readAll()).object()[sha].toArray().at(1).toString();
            if (lookup->error() == QNetworkReply::NoError && !image.isEmpty())
                downloadArchiveThumbnail(world, images.resolved(QUrl(image + ".jpg")));
        });
    });
}

void WorldsDialog::downloadArchiveThumbnail(int world, const QUrl &url) {
    QNetworkReply *reply = m_net->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, world]() {
        reply->deleteLater();
        QImage image;
        QByteArray png;
        QBuffer buffer(&png);
        if (reply->error() != QNetworkReply::NoError || !image.loadFromData(reply->readAll())
            || !buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "PNG")) return;
        QNetworkRequest request(QUrl(m_url + "/api/worlds/thumbnail?id=" + QString::number(world)));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "image/png");
        QNetworkReply *upload = m_net->post(request, png);
        connect(upload, &QNetworkReply::finished, upload, &QObject::deleteLater);
    });
}

void WorldsDialog::exportWorld() {
    QListWidgetItem *item = ui->worldsList->currentItem();
    if (!item) return;

    QString name = item->data(NameRole).toString();
    name.replace(QRegularExpression(R"([\\/:*?"<>|])"), "_");
    QString path = QFileDialog::getSaveFileName(this, "Export World", name + ".kgmap", "KoGaMa maps (*.kgmap)");
    if (path.isEmpty()) return;

    QUrl url(m_url + "/api/worlds/export");
    QUrlQuery query;
    query.addQueryItem("id", QString::number(item->data(Qt::UserRole).toInt()));
    url.setQuery(query);

    QNetworkReply *reply = m_net->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, path]() {
        reply->deleteLater();
        QFile file(path);
        if (reply->error() != QNetworkReply::NoError || !file.open(QIODevice::WriteOnly) || file.write(reply->readAll()) < 0)
            QMessageBox::warning(this, "Export", "Could not export this world");
    });
}

void WorldsDialog::renameWorld() {
    QListWidgetItem *item = ui->worldsList->currentItem();
    if (!item || item->isHidden()) return;
    ui->worldsList->editItem(item);
}

void WorldsDialog::onWorldRenamed(QListWidgetItem *item) {
    QString name = item->text().trimmed();
    QString previous = item->data(NameRole).toString();
    if (name == previous) return;
    if (name.isEmpty()) {
        item->setText(previous);
        return;
    }

    item->setData(NameRole, name);
    updateWorldActions();
    int id = item->data(Qt::UserRole).toInt();
    QUrl url(m_url + "/api/worlds/rename");
    QUrlQuery query;
    query.addQueryItem("id", QString::number(id));
    query.addQueryItem("name", name);
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
    QNetworkReply *reply = m_net->post(request, QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, reply, id]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            QMessageBox::warning(this, "Rename", "Could not rename this world");
            loadWorlds(id);
        }
    });
}

void WorldsDialog::deleteWorld() {
    QListWidgetItem *item = ui->worldsList->currentItem();
    if (!item) return;

    auto answer = QMessageBox::question(this, "Delete world",
        "Delete \"" + item->text() + "\"? This can't be undone.",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) return;

    QUrl url(m_url + "/api/worlds/delete");
    QUrlQuery query;
    query.addQueryItem("id", QString::number(item->data(Qt::UserRole).toInt()));
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
    QNetworkReply *reply = m_net->post(request, QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError)
            QMessageBox::warning(this, "Delete world", "This world is open in the game. Close it first.");
        loadWorlds();
    });
}

void WorldsDialog::showWorldMenu(const QPoint &pos) {
    QListWidgetItem *item = ui->worldsList->itemAt(pos);
    if (!item) return;
    ui->worldsList->setCurrentItem(item);

    QMenu menu(this);
    QAction *play = menu.addAction("Play");
    QAction *build = menu.addAction("Build");
    menu.addSeparator();
    QAction *exported = menu.addAction("Export...");
    QAction *rename = menu.addAction("Rename");
    QAction *remove = menu.addAction("Delete");

    QAction *chosen = menu.exec(ui->worldsList->viewport()->mapToGlobal(pos));
    if (chosen == play) launch("play", selectedWorld());
    else if (chosen == build) launch("edit", selectedWorld());
    else if (chosen == exported) exportWorld();
    else if (chosen == rename) renameWorld();
    else if (chosen == remove) deleteWorld();
}

void WorldsDialog::loadAvatars(int select) {
    if (ui->avatarsList->count() == 0) {
        ui->avatarStatusLabel->setText("Loading your avatars...");
        ui->avatarStatusLabel->show();
    }
    QNetworkReply *reply = m_net->get(QNetworkRequest(QUrl(m_url + "/api/avatars")));
    connect(reply, &QNetworkReply::finished, this, [this, reply, select, url = m_url]() {
        reply->deleteLater();
        if (url != m_url) return;
        if (reply->error() != QNetworkReply::NoError) {
            ui->avatarStatusLabel->setText("Could not load your avatars. Press F5 to try again.");
            ui->avatarStatusLabel->show();
            return;
        }
        const QJsonArray avatars = QJsonDocument::fromJson(reply->readAll()).array();
        QString version = QString::number(QDateTime::currentMSecsSinceEpoch());

        QList<int> ids;
        for (const QJsonValue &value : avatars)
            ids.append(value.toObject()["id"].toInt());
        QList<int> shown;
        for (int i = 0; i < ui->avatarsList->count(); ++i)
            shown.append(ui->avatarsList->item(i)->data(Qt::UserRole).toInt());
        QSignalBlocker blocker(ui->avatarsList);
        if (ids != shown) {
            ui->avatarsList->clear();
            const QIcon empty = TileGrid::icon(TileGrid::placeholder(TileGrid::SquareImage, palette().color(QPalette::Button)));
            for (int id : std::as_const(ids)) {
                auto *item = new QListWidgetItem(empty, QString(), ui->avatarsList);
                item->setFlags(item->flags() | Qt::ItemIsEditable);
                item->setData(Qt::UserRole, id);
                item->setToolTip("No picture yet, take one with the camera in the avatar editor");
            }
        }

        for (int i = 0; i < avatars.size(); ++i) {
            bool active = avatars[i].toObject()["active"].toBool();
            QListWidgetItem *item = ui->avatarsList->item(i);
            item->setData(ActiveRole, active);
            QString name = avatars[i].toObject()["name"].toString();
            QString text = name.isEmpty() ? "Avatar " + QString::number(ids[i]) : name;
            if (item->text() != text) {
                item->setText(text);
                item->setData(NameRole, text);
            }
            QFont font = item->font();
            if (font.bold() != active) {
                font.setBold(active);
                item->setFont(font);
            }
            loadAvatarPicture(item, ids[i], version);
            if (ids[i] == select || (select == 0 && active))
                ui->avatarsList->setCurrentItem(item);
        }
        if (!ui->avatarsList->currentItem() && ui->avatarsList->count() > 0)
            ui->avatarsList->setCurrentRow(0);
        ui->avatarStatusLabel->setText(!m_hasAvatarEditor ? "This game version has no avatar editor. Choose another version to edit characters."
            : avatars.isEmpty() ? "No avatars yet. Open the avatar editor to create your character." : QString());
        ui->avatarStatusLabel->setVisible(!ui->avatarStatusLabel->text().isEmpty());
        updateAvatarActions();
    });
}

void WorldsDialog::loadAvatarPicture(QListWidgetItem *item, int avatar, const QString &version) {
    QPointer<QListWidget> list = ui->avatarsList;
    QNetworkReply *reply = m_net->get(QNetworkRequest(QUrl(m_url + "/images/1/" + QString::number(avatar) + ".png?v=" + version)));
    connect(reply, &QNetworkReply::finished, this, [reply, list, item, avatar]() {
        reply->deleteLater();
        QByteArray data = reply->readAll();
        QPixmap image;
        if (reply->error() != QNetworkReply::NoError || !list) return;
        for (int i = 0; i < list->count(); ++i) {
            if (list->item(i) != item || item->data(Qt::UserRole).toInt() != avatar || item->data(ImageRole).toByteArray() == data) continue;
            if (!image.loadFromData(data)) return;
            item->setData(ImageRole, data);
            item->setIcon(TileGrid::icon(TileGrid::cover(image, TileGrid::SquareImage)));
            item->setToolTip(QString());
        }
    });
}

void WorldsDialog::useAvatar() {
    int avatar = selectedAvatar();
    if (avatar != 0) activateAvatar(avatar, {});
}

void WorldsDialog::editAvatar() {
    QListWidgetItem *item = ui->avatarsList->currentItem();
    if (!item || item->data(ActiveRole).toBool())
        launch("avatar", 0);
    else
        activateAvatar(selectedAvatar(), [this]() { launch("avatar", 0); });
}

void WorldsDialog::activateAvatar(int avatar, std::function<void()> done) {
    QUrl url(m_url + "/api/avatars/active");
    QUrlQuery query;
    query.addQueryItem("id", QString::number(avatar));
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
    QNetworkReply *reply = m_net->post(request, QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, reply, avatar, done]() {
        reply->deleteLater();
        bool ok = reply->error() == QNetworkReply::NoError;
        if (!ok)
            QMessageBox::warning(this, "Avatars", "Could not switch to this avatar");
        loadAvatars(avatar);
        if (ok && done) done();
    });
}

void WorldsDialog::importAvatar() {
    QString path = QFileDialog::getOpenFileName(this, "Import Avatar", QString(), "KoGaMa avatars (*.kgavatar *.json)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Import", "Could not read " + path);
        return;
    }

    QNetworkRequest request(QUrl(m_url + "/api/avatars/import"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = m_net->post(request, file.readAll());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        int id = QJsonDocument::fromJson(reply->readAll()).object()["id"].toInt();
        if (id == 0) QMessageBox::warning(this, "Import", "This file is not an avatar");
        loadAvatars(id);
    });
}

void WorldsDialog::exportAvatar() {
    int avatar = selectedAvatar();
    if (avatar == 0) return;

    QString path = QFileDialog::getSaveFileName(this, "Export Avatar", ui->avatarsList->currentItem()->text() + ".kgavatar", "KoGaMa avatars (*.kgavatar)");
    if (path.isEmpty()) return;

    QUrl url(m_url + "/api/avatars/export");
    QUrlQuery query;
    query.addQueryItem("id", QString::number(avatar));
    url.setQuery(query);

    QNetworkReply *reply = m_net->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, path]() {
        reply->deleteLater();
        QFile file(path);
        if (reply->error() != QNetworkReply::NoError || !file.open(QIODevice::WriteOnly) || file.write(reply->readAll()) < 0)
            QMessageBox::warning(this, "Export", "Could not export this avatar");
    });
}

void WorldsDialog::renameAvatar() {
    if (QListWidgetItem *item = ui->avatarsList->currentItem())
        ui->avatarsList->editItem(item);
}

void WorldsDialog::onAvatarRenamed(QListWidgetItem *item) {
    QString name = item->text().trimmed();
    QString previous = item->data(NameRole).toString();
    if (name == previous) return;
    if (name.isEmpty()) {
        QSignalBlocker blocker(ui->avatarsList);
        item->setText(previous);
        return;
    }

    item->setData(NameRole, name);
    updateAvatarActions();
    int id = item->data(Qt::UserRole).toInt();
    QUrl url(m_url + "/api/avatars/rename");
    QUrlQuery query;
    query.addQueryItem("id", QString::number(id));
    query.addQueryItem("name", name);
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
    QNetworkReply *reply = m_net->post(request, QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, reply, id]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            QMessageBox::warning(this, "Rename", "Could not rename this avatar");
            loadAvatars(id);
        }
    });
}

void WorldsDialog::showAvatarMenu(const QPoint &pos) {
    QListWidgetItem *item = ui->avatarsList->itemAt(pos);
    if (!item) return;
    ui->avatarsList->setCurrentItem(item);

    QMenu menu(this);
    QAction *use = menu.addAction("Use");
    QAction *rename = menu.addAction("Rename");
    QAction *exported = menu.addAction("Export...");

    QAction *chosen = menu.exec(ui->avatarsList->viewport()->mapToGlobal(pos));
    if (chosen == use) useAvatar();
    else if (chosen == rename) renameAvatar();
    else if (chosen == exported) exportAvatar();
}

int WorldsDialog::selectedAvatar() const {
    QListWidgetItem *item = ui->avatarsList->currentItem();
    return item ? item->data(Qt::UserRole).toInt() : 0;
}

int WorldsDialog::selectedWorld() const {
    QListWidgetItem *item = ui->worldsList->currentItem();
    return item ? item->data(Qt::UserRole).toInt() : 0;
}

void WorldsDialog::launch(const QString &mode, int world) {
    if (mode != "avatar" && world == 0) {
        QMessageBox::information(this, "Worlds", "Select a world first");
        return;
    }
    QProcess *client = GameLauncher::start(m_path, m_exe, LocalServerUrl, mode, world, m_owner, this);
    if (!client) return;
    m_clients.append(client);
    connect(client, &QProcess::finished, this, [this, client]() { m_clients.removeAll(client); });
}
