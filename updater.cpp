#include "updater.h"
#include <QApplication>
#include <QFutureWatcher>
#include <QMessageBox>
#include <QtConcurrent>
#include <Velopack.hpp>

namespace {

const std::string Repository = "https://github.com/openkogama/openkogama-launcher";

struct Update {
    std::shared_ptr<Velopack::UpdateManager> manager;
    std::optional<Velopack::UpdateInfo> info;
};

std::shared_ptr<Velopack::UpdateManager> managerFor(bool prerelease) {
    return std::make_shared<Velopack::UpdateManager>(std::make_unique<Velopack::GithubSource>(Repository, "", prerelease));
}

Update download() {
    Update update;
    try {
        update.manager = managerFor(false);
    } catch (const std::exception &) {
        return update;
    }
    try {
        if (update.manager->GetCurrentVersion().find('-') != std::string::npos)
            update.manager = managerFor(true);
        update.info = update.manager->CheckForUpdates();
        if (update.info)
            update.manager->DownloadUpdates(*update.info);
    } catch (const std::exception &error) {
        qWarning("Update failed: %s", error.what());
        update.info.reset();
    }
    return update;
}

}

void Updater::startup() {
    Velopack::VelopackApp::Build().Run();
}

void Updater::check(QWidget *window) {
    auto *watcher = new QFutureWatcher<Update>(window);
    QObject::connect(watcher, &QFutureWatcher<Update>::finished, window, [watcher, window]() {
        Update update = watcher->result();
        watcher->deleteLater();
        if (!update.info)
            return;
        QString version = QString::fromStdString(update.info->TargetFullRelease.Version);
        if (QMessageBox::question(window, "Update", "OpenKogama Launcher " + version + " is ready. Restart now to update?") != QMessageBox::Yes)
            return;
        try {
            update.manager->WaitExitThenApplyUpdates(*update.info);
            QApplication::quit();
        } catch (const std::exception &error) {
            QMessageBox::warning(window, "Update", QString::fromUtf8(error.what()));
        }
    });
    watcher->setFuture(QtConcurrent::run(download));
}
