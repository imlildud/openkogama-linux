#include "theme.h"
#include <QApplication>
#include <QFile>
#include <QHash>
#include <QStyleHints>

namespace {

const QHash<QString, QString> DarkColors = {
    {"text", "#ffffff"},
    {"muted", "#9d9d9d"},
    {"button", "#3c3c3c"},
    {"border", "#4a4a4a"},
    {"hover", "#464646"},
    {"hover-border", "#5a5a5a"},
    {"pressed", "#333333"},
    {"disabled", "#2a2a2a"},
    {"disabled-text", "#6e6e6e"},
    {"disabled-border", "#333333"},
    {"field", "#2d2d2d"},
    {"alternate", "#262626"},
    {"separator", "#3c3c3c"},
    {"scroll", "#4a4a4a"},
    {"scroll-hover", "#5a5a5a"},
    {"check-border", "#5a5a5a"},
    {"plus", ":/plus.svg"},
    {"arrow", ":/arrow-down.svg"},
};

const QHash<QString, QString> LightColors = {
    {"text", "#1b1b1b"},
    {"muted", "#666666"},
    {"button", "#f3f3f3"},
    {"border", "#d4d4d4"},
    {"hover", "#e9e9e9"},
    {"hover-border", "#bdbdbd"},
    {"pressed", "#e0e0e0"},
    {"disabled", "#f6f6f6"},
    {"disabled-text", "#a3a3a3"},
    {"disabled-border", "#e3e3e3"},
    {"field", "#ffffff"},
    {"alternate", "#f7f7f7"},
    {"separator", "#e3e3e3"},
    {"scroll", "#c6c6c6"},
    {"scroll-hover", "#a6a6a6"},
    {"check-border", "#a6a6a6"},
    {"plus", ":/plus-light.svg"},
    {"arrow", ":/arrow-down-light.svg"},
};

QString styleSheet() {
    QFile file(":/launcher.qss");
    if (!file.open(QIODevice::ReadOnly))
        return {};
    QString style = QString::fromUtf8(file.readAll());
    const QHash<QString, QString> &colors = Theme::dark() ? DarkColors : LightColors;
    for (auto it = colors.begin(); it != colors.end(); ++it)
        style.replace("@" + it.key() + "@", it.value());
    return style;
}

}

void Theme::apply(QApplication &app) {
    app.setStyleSheet(styleSheet());
    QObject::connect(app.styleHints(), &QStyleHints::colorSchemeChanged, &app, [&app]() {
        app.setStyleSheet(styleSheet());
    });
}

bool Theme::dark() {
    return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
}
