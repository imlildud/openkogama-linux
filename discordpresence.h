#pragma once

#include <QString>

class QProcess;

namespace DiscordPresence {
void show(QProcess *client, const QString &details, const QString &state);
void refresh();
}
