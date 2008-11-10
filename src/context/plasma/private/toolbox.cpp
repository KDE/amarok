/*
 *   Copyright 2007 by Aaron Seigo <aseigo@kde.org>
 *   Copyright 2008 by Marco Martin <notmart@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "toolbox_p.h"

#include <QAction>
#include <QGraphicsSceneHoverEvent>
#include <QPainter>
#include <QRadialGradient>

#include <KColorScheme>
#include <KDebug>

#include <plasma/theme.h>
#include "widgets/iconwidget.h"

namespace Plasma
{

class ToolBoxPrivate
{
public:
    ToolBoxPrivate()
      : size(50),
      iconSize(32, 32),
      hidden(false),
      showing(false),
      corner(ToolBox::TopRight)
    {}

    int size;
    QSize iconSize;
    bool hidden;
    bool showing;
    ToolBox::Corner corner;
};

ToolBox::ToolBox(QGraphicsItem *parent)
    : QGraphicsItem(parent),
      d(new ToolBoxPrivate)
{
    setAcceptsHoverEvents(true);
}

ToolBox::~ToolBox()
{
    delete d;
}

QPoint ToolBox::toolPosition(int toolHeight)
{
    switch (d->corner) {
    case TopRight:
        return QPoint(d->size * 2, -toolHeight);
    case Top:
        return QPoint((int)boundingRect().center().x() - d->iconSize.width(), -toolHeight);
    case TopLeft:
        return QPoint(-d->size * 2, -toolHeight);
    case Left:
        return QPoint(-d->size * 2, (int)boundingRect().center().y() - d->iconSize.height());
    case Right:
        return QPoint(d->size * 2, (int)boundingRect().center().y() - d->iconSize.height());
    case BottomLeft:
        return QPoint(-d->size * 2, toolHeight);
    case Bottom:
        return QPoint((int)boundingRect().center().x() - d->iconSize.width(), toolHeight);
    case BottomRight:
    default:
        return QPoint(d->size * 2, toolHeight);
    }
}

void ToolBox::addTool(QAction *action)
{
    if (!action) {
        return;
    }

    Plasma::IconWidget *tool = new Plasma::IconWidget(this);

    tool->setAction(action);
    tool->setDrawBackground(true);
    tool->setOrientation(Qt::Horizontal);
    tool->resize(tool->sizeFromIconSize(22));

    tool->hide();
    const int height = static_cast<int>(tool->boundingRect().height());
    tool->setPos(toolPosition(height));
    tool->setZValue(zValue() + 1);

    //make enabled/disabled tools appear/disappear instantly
    connect(tool, SIGNAL(changed()), this, SLOT(updateToolBox()));
}

void ToolBox::updateToolBox()
{
    if (d->showing) {
        d->showing = false;
        showToolBox();
    }
}

void ToolBox::removeTool(QAction *action)
{
    foreach (QGraphicsItem *child, QGraphicsItem::children()) {
        //kDebug() << "checking tool" << child << child->data(ToolName);
        Plasma::IconWidget *tool = dynamic_cast<Plasma::IconWidget*>(child);
        if (tool && tool->action() == action) {
            //kDebug() << "tool found!";
            tool->deleteLater();
            break;
        }
    }
}

int ToolBox::size() const
{
    return  d->size;
}

void ToolBox::setSize(const int newSize)
{
    d->size = newSize;
}

QSize ToolBox::iconSize() const
{
    return d->iconSize;
}

void ToolBox::setIconSize(const QSize newSize)
{
    d->iconSize = newSize;
}

bool ToolBox::showing() const
{
    return  d->showing;
}

void ToolBox::setShowing(const bool show)
{
    d->showing = show;
}

void ToolBox::setCorner(const Corner corner)
{
    d->corner = corner;
}

ToolBox::Corner ToolBox::corner() const
{
    return d->corner;
}

void ToolBox::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    event->accept();
}

void ToolBox::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (boundingRect().contains(event->pos())) {
        emit toggled();
    }
}

} // plasma namespace

#include "toolbox_p.moc"

