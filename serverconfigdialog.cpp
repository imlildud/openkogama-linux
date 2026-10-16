#include "serverconfigdialog.h"
#include "paths.h"
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QJsonDocument>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QStyle>
#include <QUrl>
#include <QVBoxLayout>

namespace {
constexpr auto Reference = "https://github.com/openkogama/openkogama-server#launcher-servers";
const QStringList Fixed = {"httpPort", "photonPort", "publicUrl", "listenHost"};
const QString Starter = R"({
    "title": "",
    "joinMode": "play",
    "clients": [],
    "mixedClients": false,
    "keepPlayChanges": false
}
)";

QString configPath()
{
    return QDir(Paths::serverData()).filePath("server.json");
}
}

QJsonObject ServerConfigDialog::current()
{
    QFile file(configPath());
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QJsonDocument::fromJson(file.readAll()).object();
}

ServerConfigDialog::ServerConfigDialog(QWidget *parent)
    : QDialog(parent), m_text(new QPlainTextEdit(this)), m_error(new QLabel(this))
{
    setWindowTitle("Server settings");
    resize(520, 420);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);
    auto *hint = new QLabel(QString("The server.json of the server on this computer, which hosted worlds follow. "
        "Saving restarts the server. <a href=\"%1\">All settings</a>").arg(QLatin1String(Reference)), this);
    hint->setWordWrap(true);
    hint->setOpenExternalLinks(true);
    layout->addWidget(hint);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    mono.setPointSizeF(font().pointSizeF());
    m_text->setObjectName("serverConfigEdit");
    m_text->setFont(mono);
    m_text->setLineWrapMode(QPlainTextEdit::NoWrap);
    QFile file(configPath());
    m_text->setPlainText(file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : Starter);
    layout->addWidget(m_text, 1);
    m_error->setObjectName("serverConfigError");
    m_error->setWordWrap(true);
    m_error->hide();
    layout->addWidget(m_error);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    auto *folder = buttons->addButton("Open folder", QDialogButtonBox::ResetRole);
    auto *confirm = buttons->button(QDialogButtonBox::Save);
    confirm->setProperty("primary", true);
    confirm->style()->polish(confirm);
    connect(folder, &QPushButton::clicked, this, []() {
        QDir().mkpath(Paths::serverData());
        QDesktopServices::openUrl(QUrl::fromLocalFile(Paths::serverData()));
    });
    connect(buttons, &QDialogButtonBox::accepted, this, &ServerConfigDialog::save);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

void ServerConfigDialog::save()
{
    QJsonParseError error;
    QByteArray text = m_text->toPlainText().toUtf8();
    const QJsonDocument document = QJsonDocument::fromJson(text, &error);
    QString problem;
    if (error.error != QJsonParseError::NoError)
        problem = "Line " + QString::number(text.left(error.offset).count('\n') + 1) + ": " + error.errorString();
    else if (!document.isObject())
        problem = "The settings have to be one { } object.";
    for (const QString &key : Fixed)
        if (problem.isEmpty() && document.object().contains(key))
            problem = "Leave out " + key + ", the launcher reaches this server on 127.0.0.1:8080.";
    if (!problem.isEmpty()) {
        m_error->setText(problem);
        m_error->show();
        return;
    }
    if (!text.endsWith('\n')) text += '\n';
    QDir().mkpath(Paths::serverData());
    QSaveFile file(configPath());
    if (!file.open(QIODevice::WriteOnly) || file.write(text) < 0 || !file.commit()) {
        m_error->setText("Could not save " + QDir::toNativeSeparators(configPath()));
        m_error->show();
        return;
    }
    accept();
}
