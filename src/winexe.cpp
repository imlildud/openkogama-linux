#include "winexe.h"
#include <QMessageBox>
#include <QProcess>
#include <QStandardPaths>

namespace WinExe {

static QString wine() {
    static const QString path = QStandardPaths::findExecutable("wine");
    return path;
}

bool available() {
#ifdef Q_OS_WIN
    return true;
#else
    return !wine().isEmpty();
#endif
}

void setCommand(QProcess *process, const QString &exe, const QStringList &arguments) {
#ifdef Q_OS_WIN
    process->setProgram(exe);
    process->setArguments(arguments);
#else
    static bool warned = false;
    if (wine().isEmpty()) {
        if (!warned) {
            warned = true;
            QMessageBox::warning(nullptr, "Wine",
                "Wine is not installed.\nInstall wine to run KoGaMa on Linux.");
        }
        process->setProgram(exe);
        process->setArguments(arguments);
        return;
    }
    process->setProgram(wine());
    process->setArguments(QStringList{exe} + arguments);
#endif
}

}
