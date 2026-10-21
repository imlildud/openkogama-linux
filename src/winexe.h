#ifndef WINEXE_H
#define WINEXE_H

class QProcess;
class QString;
class QStringList;

namespace WinExe {

void setCommand(QProcess *process, const QString &exe, const QStringList &arguments);
bool available();

}

#endif // WINEXE_H
