#include "tilegrid.h"
#include "centereddelegate.h"
#include <QListWidget>
#include <QScrollBar>
#include <QPainter>
#include <QStyle>
#include <algorithm>

namespace {
constexpr int Spacing = 16;
constexpr int TextHeight = 40;
}

namespace TileGrid {

void setup(QListWidget *list, QObject *resizeFilter)
{
    list->setItemDelegate(new CenteredDelegate(list));
    list->setViewMode(QListView::IconMode);
    list->setResizeMode(QListView::Adjust);
    list->setMovement(QListView::Static);
    list->setWordWrap(true);
    list->setUniformItemSizes(true);
    list->setFrameShape(QFrame::NoFrame);
    list->setFocusPolicy(Qt::StrongFocus);
    list->setEditTriggers(QAbstractItemView::NoEditTriggers);
    list->setContextMenuPolicy(Qt::CustomContextMenu);
    list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    list->setTextElideMode(Qt::ElideRight);
    list->installEventFilter(resizeFilter);
}

void fit(QListWidget *list, const QSize &tile)
{
    int available = list->contentsRect().width() - list->style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, list->verticalScrollBar()) - 1;
    int columns = std::max(1, (available + Spacing) / (tile.width() + Spacing));
    int column = std::max(tile.width(), available / columns);
    int width = std::max(tile.width(), (column * columns - Spacing * (columns - 1)) / columns);
    QSize scaled(width, width * tile.height() / tile.width());
    QSize grid(column, scaled.height() + TextHeight + Spacing);
    if (list->iconSize() != scaled) list->setIconSize(scaled);
    if (list->gridSize() != grid) list->setGridSize(grid);
}

QPixmap placeholder(const QSize &size, const QColor &background)
{
    QPixmap pixmap(size);
    pixmap.fill(background);
    QPainter painter(&pixmap);
    painter.setOpacity(0.3);
    QPixmap logo = QPixmap(":/logo.png").scaled(size * 0.6, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    painter.drawPixmap((size.width() - logo.width()) / 2, (size.height() - logo.height()) / 2, logo);
    return pixmap;
}

QPixmap cover(const QPixmap &image, const QSize &size)
{
    QPixmap scaled = image.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    return scaled.copy((scaled.width() - size.width()) / 2, (scaled.height() - size.height()) / 2, size.width(), size.height());
}

QIcon icon(const QPixmap &pixmap)
{
    QIcon result;
    result.addPixmap(pixmap, QIcon::Normal);
    result.addPixmap(pixmap, QIcon::Selected);
    return result;
}

}
