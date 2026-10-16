#include "sharebar.h"
#include "settings.h"
#include "tunnel.h"
#include <QClipboard>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>
#include <QTimer>

namespace {
constexpr int SlowMs = 10000;
constexpr int CopiedMs = 1500;
constexpr int Gap = 16;
}

ShareBar::ShareBar(QWidget *parent)
    : QFrame(parent), m_text(new QLabel(this)), m_code(new QLabel(this)), m_info(new QLabel(this)),
      m_copy(new QPushButton("Copy code", this)),
      m_stop(new QPushButton(this)), m_slow(new QTimer(this))
{
    setObjectName("shareBar");
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 3, 4, 3);
    layout->setSpacing(8);
    m_text->setObjectName("shareText");
    m_code->setObjectName("shareCode");
    m_info->setObjectName("shareInfo");
    for (QLabel *label : {m_text, m_code, m_info}) {
        label->setTextFormat(Qt::PlainText);
        label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    }
    m_code->setContentsMargins(Gap, 0, 0, 0);
    m_info->setContentsMargins(Gap, 0, 0, 0);
    m_copy->setObjectName("shareCopyButton");
    m_stop->setObjectName("shareStopButton");
    layout->addWidget(m_text);
    layout->addWidget(m_code);
    layout->addWidget(m_info);
    layout->addStretch(1);
    layout->addWidget(m_copy);
    layout->addWidget(m_stop);
    m_slow->setSingleShot(true);
    m_slow->setInterval(SlowMs);
    connect(m_slow, &QTimer::timeout, this, [this]() {
        m_waiting = true;
        refresh();
    });
    connect(m_copy, &QPushButton::clicked, this, [this]() {
        QGuiApplication::clipboard()->setText(Tunnel::display(m_codeText));
        m_copy->setText("Copied");
        QTimer::singleShot(CopiedMs, m_copy, [this]() { m_copy->setText("Copy code"); });
    });
    connect(m_stop, &QPushButton::clicked, this, &ShareBar::stop);
    hide();
}

void ShareBar::share(int world, const QString &name, const QString &version)
{
    if (m_tunnel) {
        Tunnel *old = m_tunnel;
        m_tunnel = nullptr;
        old->stop();
        old->deleteLater();
    }
    m_world = world;
    m_name = name;
    m_version = version;
    m_codeText.clear();
    m_error.clear();
    m_players = 0;
    if (version == "2012") m_error = "KoGaMa 2012 can't be hosted with a code yet";
    else start();
    refresh();
    show();
}

void ShareBar::setVersion(const QString &version)
{
    if (version != m_version && m_tunnel) share(m_world, m_name, version);
}

void ShareBar::stop()
{
    if (m_tunnel) {
        Tunnel *tunnel = m_tunnel;
        m_tunnel = nullptr;
        tunnel->stop();
        tunnel->deleteLater();
    }
    m_slow->stop();
    m_codeText.clear();
    m_error.clear();
    m_players = 0;
    hide();
}

void ShareBar::start()
{
    const QString nickname = Settings::nickname();
    const QJsonObject info{{"title", nickname.isEmpty() ? m_name : m_name + " (" + nickname + ")"},
        {"version", m_version}, {"world", m_world}};
    Tunnel *tunnel = Tunnel::host(Settings::hostSecret(), QString::fromUtf8(QJsonDocument(info).toJson(QJsonDocument::Compact)), this);
    m_tunnel = tunnel;
    connect(tunnel, &Tunnel::said, this, [this, tunnel](const QString &word, const QString &rest) {
        if (tunnel == m_tunnel) heard(word, rest);
    });
    connect(tunnel, &Tunnel::ended, this, [this, tunnel]() {
        if (tunnel == m_tunnel) ended();
    });
    m_waiting = false;
    m_slow->start();
}

void ShareBar::heard(const QString &word, const QString &rest)
{
    if (word == "code") {
        m_codeText = Tunnel::normalize(rest);
        m_slow->stop();
    } else if (word == "players") {
        m_players = rest.toInt();
    } else if (word == "error") {
        m_error = rest.startsWith("no server") ? "your local server is not running, open the Worlds tab first"
            : rest.startsWith("rendezvous") ? "the OpenKogama P2P servers can't be reached" : rest;
    }
    refresh();
}

void ShareBar::ended()
{
    Tunnel *tunnel = m_tunnel;
    m_tunnel = nullptr;
    tunnel->deleteLater();
    m_slow->stop();
    m_codeText.clear();
    m_players = 0;
    if (m_error.isEmpty()) m_error = "hosting stopped unexpectedly";
    refresh();
}

void ShareBar::refresh()
{
    const bool live = m_tunnel && !m_codeText.isEmpty();
    const bool failed = !m_tunnel && !m_error.isEmpty();
    m_text->setText(failed ? "Could not host " + m_name + ": " + m_error : "Hosting " + m_name);
    m_code->setText("Code " + Tunnel::display(m_codeText));
    m_code->setVisible(live);
    if (live) m_info->setText(m_players == 0 ? "Nobody joined yet" : m_players == 1 ? "1 player joined" : QString::number(m_players) + " players joined");
    else m_info->setText(m_waiting ? "Still reaching the P2P servers..." : "Getting a code...");
    m_info->setVisible(!failed);
    m_copy->setVisible(live);
    m_stop->setText(m_tunnel ? "Stop hosting" : "Close");
    setToolTip(m_version.isEmpty() ? QString() : "Players need KoGaMa " + m_version + " and type the code in Servers, Join with code");
}
