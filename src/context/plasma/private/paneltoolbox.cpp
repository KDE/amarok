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

#include "paneltoolbox_p.h"

#include <QGraphicsSceneHoverEvent>
#include <QPainter>
#include <QRadialGradient>
#include <QApplication>

#include <plasma/theme.h>
#include <plasma/paintutils.h>
#include <KColorScheme>

#include <KDebug>

#include <plasma/applet.h>

namespace Plasma
{

class EmptyGraphicsItem : public QGraphicsItem
{
    public:
        EmptyGraphicsItem(QGraphicsItem *parent)
            : QGraphicsItem(parent)
        {
            setAcceptsHoverEvents(true);
        }

        QRectF boundingRect() const
        {
            return QRectF(QPointF(0, 0), m_rect.size());
        }

        QRectF rect() const
        {
            return m_rect;
        }

        void setRect(const QRectF &rect)
        {
            //kDebug() << "setting rect to" << rect;
            prepareGeometryChange();
            m_rect = rect;
            setPos(rect.topLeft());
        }

        void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *)
        {
            Q_UNUSED(p)
            //p->setPen(Qt::red);
            //p->drawRect(boundingRect());
        }

    private:
        QRectF m_rect;
};

// used with QGrahphicsItem::setData
static const int ToolName = 7001;

class PanelToolBoxPrivate
{
public:
    PanelToolBoxPrivate()
      : icon("plasma"),
        toolBacker(0),
        animId(0),
        animFrame(0),
        toggled(false)
    {}

    KIcon icon;
    EmptyGraphicsItem *toolBacker;
    QTime stopwatch;
    int animId;
    qreal animFrame;
    bool toggled;
};

PanelToolBox::PanelToolBox(QGraphicsItem *parent)
    : ToolBox(parent),
      d(new PanelToolBoxPrivate)
{
    connect(Plasma::Animator::self(), SIGNAL(movementFinished(QGraphicsItem*)),
            this, SLOT(toolMoved(QGraphicsItem*)));
    connect(this, SIGNAL(toggled()), this, SLOT(toggle()));

    setZValue(10000000);
    setFlag(ItemClipsToShape, true);
    setFlag(ItemClipsChildrenToShape, false);
    //panel toolbox is allowed to zoom, otherwise a part of it will be displayed behind the desktop
    //toolbox when the desktop is zoomed out
    setFlag(ItemIgnoresTransformations, false);
}

PanelToolBox::~PanelToolBox()
{
    delete d;
}

QRectF PanelToolBox::boundingRect() const
{
    if (corner() == ToolBox::Bottom) {
        return QRectF(0, 0, size() * 2, -size());
    } else if (corner() == ToolBox::Left) {
        return QRectF(0, 0, size(), size() * 2);
    //Only Left,Right and Bottom supported, default to Right
    } else {
        return QRectF(0, 0, -size(), size() * 2);
    }
}

void PanelToolBox::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    const qreal progress = d->animFrame / size();

    QPoint gradientCenter;
    if (corner() == ToolBox::Bottom) {
        gradientCenter = QPoint(boundingRect().center().x(), boundingRect().top());
    } else {
        gradientCenter = QPoint(boundingRect().left(), boundingRect().center().y());
    }

    {
        painter->translate(boundingRect().topLeft());

        KColorScheme colors(QPalette::Active, KColorScheme::Window,
                            Plasma::Theme::defaultTheme()->colorScheme());
        QColor color1 = colors.background().color();
        color1.setAlpha(64.0);

        QColor color2 = colors.foreground().color();
        color2.setAlpha(64.0);

        QRadialGradient gradient(gradientCenter, size() - 2);
        gradient.setFocalPoint(gradientCenter);
        gradient.setColorAt(0, color1);
        gradient.setColorAt(.85, color1);
        gradient.setColorAt(.95, color2);
        color2.setAlpha(0);
        gradient.setColorAt(1, color2);

        painter->save();
        painter->setPen(Qt::NoPen);
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setBrush(gradient);
        QPainterPath p = shape();
        painter->drawPath(p);
        painter->restore();
    }

    QRect iconRect;

    if (corner() == ToolBox::Bottom) {
        iconRect = QRect(QPoint(gradientCenter.x() - iconSize().width() / 2,
                                (int)boundingRect().top() - iconSize().height() - 2), iconSize());
    } else if (corner() == ToolBox::Left) {
        iconRect = QRect(QPoint(2, gradientCenter.y() - iconSize().height() / 2), iconSize());
    //Only Left,Right and Bottom supported, default to Right
    } else {
        iconRect = QRect(QPoint((int)boundingRect().left() - iconSize().width() + 1,
                                gradientCenter.y() - iconSize().height() / 2), iconSize());
    }

    if (qFuzzyCompare(qreal(1.0), progress)) {
        d->icon.paint(painter, iconRect);
    } else if (qFuzzyCompare(qreal(1.0), 1 + progress)) {
        d->icon.paint(painter, iconRect, Qt::AlignCenter, QIcon::Disabled, QIcon::Off);
    } else {
        QPixmap disabled = d->icon.pixmap(iconSize(), QIcon::Disabled, QIcon::Off);
        QPixmap enabled = d->icon.pixmap(iconSize());
        QPixmap result = PaintUtils::transition(
            d->icon.pixmap(iconSize(), QIcon::Disabled, QIcon::Off),
            d->icon.pixmap(iconSize()), progress);
        painter->drawPixmap(iconRect, result);
    }
}

QPainterPath PanelToolBox::shape() const
{
    QPainterPath path;
    int toolSize = size();// + (int)d->animFrame;

    if (corner() == ToolBox::Bottom) {
        path.arcTo(QRectF(boundingRect().center().x() - toolSize,
                          boundingRect().top() - toolSize,
                          toolSize * 2,
                          toolSize * 2), 0, 180);
    } else if (corner() == ToolBox::Left) {
        path.arcTo(QRectF(boundingRect().left() - toolSize,
                          boundingRect().center().y() - toolSize,
                          toolSize * 2,
                          toolSize * 2), 90, -180);
    //Only Left,Right and Bottom supported, default to Right
    } else {
        path.arcTo(QRectF(boundingRect().left() - toolSize,
                          boundingRect().center().y() - toolSize,
                          toolSize * 2,
                          toolSize * 2), 90, 180);
    }

    return path;
}

void PanelToolBox::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    if (showing() || d->stopwatch.elapsed() < 100) {
        QGraphicsItem::hoverEnterEvent(event);
        return;
    }

    showToolBox();
    QGraphicsItem::hoverEnterEvent(event);
}

void PanelToolBox::showToolBox()
{
    if (showing()) {
        return;
    }

    int maxwidth = 0;
    foreach (QGraphicsItem *tool, QGraphicsItem::children()) {
        if (!tool->isEnabled()) {
            continue;
        }
        maxwidth = qMax(static_cast<int>(tool->boundingRect().width()), maxwidth);
    }

    // put tools 5px from icon edge
    const int iconWidth = 32;
    int x = size() * 2 - maxwidth - iconWidth - 5;
    int y = 5; // pos().y();
    Plasma::Animator *animdriver = Plasma::Animator::self();
    foreach (QGraphicsItem *tool, QGraphicsItem::children()) {
        if (tool == d->toolBacker) {
            continue;
        }

        if (!tool->isEnabled()) {
            if (tool->isVisible()) {
                const int height = static_cast<int>(tool->boundingRect().height());
                animdriver->moveItem(tool, Plasma::Animator::SlideOutMovement,
                                     QPoint(size() * 2, -height));
            }
            continue;
        }

        //kDebug() << "let's show and move" << tool << tool->boundingRect();
        tool->show();
        animdriver->moveItem(tool, Plasma::Animator::SlideInMovement, QPoint(x, y));
        //x += 0;
        y += static_cast<int>(tool->boundingRect().height()) + 5;
    }

    if (!d->toolBacker) {
        d->toolBacker = new EmptyGraphicsItem(this);
    }
    d->toolBacker->setRect(QRectF(QPointF(x, 0), QSizeF(maxwidth, y - 10)));
    d->toolBacker->show();

    if (d->animId) {
        animdriver->stopCustomAnimation(d->animId);
    }

    setShowing(true);
    // TODO: 10 and 200 shouldn't be hardcoded here. There needs to be a way to
    // match whatever the time is that moveItem() takes. Same in hoverLeaveEvent().
    d->animId = animdriver->customAnimation(
        10, 240, Plasma::Animator::EaseInCurve, this, "animate");
    d->stopwatch.restart();
}

void PanelToolBox::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    //kDebug() << event->pos() << event->scenePos()
    //         << d->toolBacker->rect().contains(event->scenePos().toPoint());
    if ((d->toolBacker && d->toolBacker->rect().contains(event->scenePos().toPoint())) ||
        d->stopwatch.elapsed() < 100 || d->toggled) {
        QGraphicsItem::hoverLeaveEvent(event);
        return;
    }

    hideToolBox();
    QGraphicsItem::hoverLeaveEvent(event);
}

void PanelToolBox::hideToolBox()
{
    if (!showing()) {
        return;
    }

    d->toggled = false;
    int x = size() * 2;
    int y = 0;
    Plasma::Animator *animdriver = Plasma::Animator::self();
    foreach (QGraphicsItem *tool, QGraphicsItem::children()) {
        if (tool == d->toolBacker) {
            continue;
        }

        const int height = static_cast<int>(tool->boundingRect().height());
        animdriver->moveItem(tool, Plasma::Animator::SlideOutMovement, QPoint(x, y-height));
    }

    if (d->animId) {
        animdriver->stopCustomAnimation(d->animId);
    }

    setShowing(false);
    d->animId = animdriver->customAnimation(
        10, 240, Plasma::Animator::EaseOutCurve, this, "animate");

    if (d->toolBacker) {
        d->toolBacker->hide();
    }

    d->stopwatch.restart();
}

void PanelToolBox::animate(qreal progress)
{
    if (showing()) {
        d->animFrame = size() * progress;
    } else {
        d->animFrame = size() * (1.0 - progress);
    }

    //kDebug() << "animating at" << progress << "for" << d->animFrame;

    if (progress >= 1) {
        d->animId = 0;
    }

    update();
}

void PanelToolBox::toolMoved(QGraphicsItem *item)
{
    //kDebug() << "geometry is now " << static_cast<Plasma::Widget*>(item)->geometry();
    if (!showing() &&
        QGraphicsItem::children().indexOf(static_cast<Plasma::Applet*>(item)) != -1) {
        item->hide();
    }
}

void PanelToolBox::toggle()
{
    d->toggled = !d->toggled;
    if (showing() && !d->toggled) {
        hideToolBox();
    }
}

} // plasma namespace

#include "paneltoolbox_p.moc"

