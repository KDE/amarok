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

#include "desktoptoolbox_p.h"

#include <QGraphicsSceneHoverEvent>
#include <QPainter>
#include <QRadialGradient>
#include <QGraphicsView>

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

class DesktopToolBoxPrivate
{
public:
    DesktopToolBoxPrivate()
      : icon("plasma"),
        toolBacker(0),
        animCircleId(0),
        animHighlightId(0),
        animCircleFrame(0),
        animHighlightFrame(0),
        hovering(0)
    {}

    KIcon icon;
    EmptyGraphicsItem *toolBacker;
    int animCircleId;
    int animHighlightId;
    qreal animCircleFrame;
    qreal animHighlightFrame;
    QRect shapeRect;
    bool hovering : 1;
};

DesktopToolBox::DesktopToolBox(QGraphicsItem *parent)
    : ToolBox(parent),
      d(new DesktopToolBoxPrivate)
{
    connect(Plasma::Animator::self(), SIGNAL(movementFinished(QGraphicsItem*)),
            this, SLOT(toolMoved(QGraphicsItem*)));
    connect(this, SIGNAL(toggled()), this, SLOT(toggle()));

    setZValue(10000000);
    setFlag(ItemClipsToShape, true);
    setFlag(ItemClipsChildrenToShape, false);
    setFlag(ItemIgnoresTransformations, true);
}

DesktopToolBox::~DesktopToolBox()
{
    delete d;
}

QRectF DesktopToolBox::boundingRect() const
{
    Corner c = corner();
    qreal width;
    qreal height;

    if (c == Left || c == Right) {
        height = size() * 4;
    } else {
        height = size() * 2;
    }

    if (c == Bottom || c == BottomRight || c == BottomLeft) {
        height = -height;
    }

    if (c == Top || c == Bottom) {
        width = size() * 4;
    } else {
        width = size() * 2;
    }

    if (c == Right || c == TopRight || c == BottomRight) {
        width = -width;
    }

    return QRectF(0, 0, width, height);
}

void DesktopToolBox::paint(QPainter *painter,
                           const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    painter->save();
    painter->translate(boundingRect().topLeft());

    QColor color1 = KColorScheme(QPalette::Active, KColorScheme::Window,
                                 Plasma::Theme::defaultTheme()->colorScheme()).background().color();
    color1.setAlpha(64);

    QColor color2 = KColorScheme(QPalette::Active, KColorScheme::Window,
                                 Plasma::Theme::defaultTheme()->colorScheme()).foreground().color();
    color2.setAlpha(64);

    QPainterPath p = shape();

    QPoint iconPos;
    QPointF gradientCenter;
    switch (corner()) {
    case TopRight:
        iconPos = QPoint((int)boundingRect().left() - iconSize().width() + 2, 2);
        gradientCenter = boundingRect().topLeft();
        break;
    case Top:
        iconPos = QPoint(boundingRect().center().x() - iconSize().width() / 2, 2);
        gradientCenter = QPoint(boundingRect().center().x(), boundingRect().y());
        break;
    case TopLeft:
        iconPos = QPoint(2, 2);
        gradientCenter = boundingRect().topLeft();
        break;
    case Left:
        iconPos = QPoint(2, boundingRect().center().y() - iconSize().height() / 2);
        gradientCenter = QPointF(boundingRect().left(), boundingRect().center().y());
        break;
    case Right:
        iconPos = QPoint((int)boundingRect().left() - iconSize().width() + 2,
                         boundingRect().center().y() - iconSize().height() / 2);
        gradientCenter = QPointF(boundingRect().left(), boundingRect().center().y());
        break;
    case BottomLeft:
        iconPos = QPoint(2, boundingRect().top() - iconSize().height() - 2);
        gradientCenter = boundingRect().topLeft();
        break;
    case Bottom:
        iconPos = QPoint(boundingRect().center().x() - iconSize().width() / 2,
                         boundingRect().top() - iconSize().height() - 2);
        gradientCenter = QPointF(boundingRect().center().x(), boundingRect().top());
        break;
    case BottomRight:
    default:
        iconPos = QPoint((int)boundingRect().left() - iconSize().width() - 2,
                         (int)boundingRect().top() - iconSize().height() - 2);
        gradientCenter = boundingRect().topLeft();
        break;
    }

    QRadialGradient gradient(gradientCenter, size() + d->animCircleFrame);
    gradient.setFocalPoint(gradientCenter);
    gradient.setColorAt(0, color1);
    gradient.setColorAt(.87, color1);
    gradient.setColorAt(.97, color2);
    color2.setAlpha(0);
    gradient.setColorAt(1, color2);
    painter->save();
    painter->setPen(Qt::NoPen);
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setBrush(gradient);
    painter->drawPath(p);
    painter->restore();

    const qreal progress = d->animHighlightFrame;

    if (qFuzzyCompare(qreal(1.0), progress)) {
        d->icon.paint(painter, QRect(iconPos, iconSize()));
    } else if (qFuzzyCompare(qreal(1.0), 1 + progress)) {
        d->icon.paint(painter, QRect(iconPos, iconSize()),
                      Qt::AlignCenter, QIcon::Disabled, QIcon::Off);
    } else {
        QPixmap disabled = d->icon.pixmap(iconSize(), QIcon::Disabled, QIcon::Off);
        QPixmap enabled = d->icon.pixmap(iconSize());
        QPixmap result = PaintUtils::transition(
            d->icon.pixmap(iconSize(), QIcon::Disabled, QIcon::Off),
            d->icon.pixmap(iconSize()), progress);
        painter->drawPixmap(QRect(iconPos, iconSize()), result);
    }

    painter->restore();
}

QPainterPath DesktopToolBox::shape() const
{
    QPainterPath path;
    int toolSize = size() + (int)d->animCircleFrame;

    switch (corner()) {
    case TopRight:
        path.arcTo(QRectF(boundingRect().left() - toolSize, boundingRect().top() - toolSize,
                          toolSize * 2, toolSize * 2), 180, 90);
        break;
    case Top:
        path.arcTo(QRectF(boundingRect().center().x() - toolSize,
                          boundingRect().top() - toolSize,
                          toolSize * 2, toolSize * 2), 180, 180);
        break;
    case TopLeft:
        path.arcTo(QRectF(boundingRect().left() - toolSize,
                          boundingRect().top() - toolSize,
                          toolSize * 2, toolSize * 2), 270, 90);
        break;
    case Left:
        path.arcTo(QRectF(boundingRect().left() - toolSize,
                          boundingRect().center().y() - toolSize,
                          toolSize * 2, toolSize * 2), 270, 180);
        break;
    case Right:
        path.arcTo(QRectF(boundingRect().left() - toolSize,
                          boundingRect().center().y() - toolSize,
                          toolSize * 2, toolSize * 2), 90, 180);
        break;
    case BottomLeft:
        path.arcTo(QRectF(boundingRect().left() - toolSize,
                          boundingRect().top() - toolSize,
                          toolSize * 2, toolSize * 2), 0, 90);
        break;
    case Bottom:
        path.arcTo(QRectF(boundingRect().center().x() - toolSize,
                          boundingRect().top() - toolSize,
                          toolSize * 2, toolSize * 2), 0, 180);
        break;
    case BottomRight:
    default:
        path.arcTo(QRectF(boundingRect().left() - toolSize,
                          boundingRect().top() - toolSize,
                          toolSize * 2, toolSize * 2), 90, 90);
        break;
    }

    return path;
}

void DesktopToolBox::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    if (showing() || d->hovering) {
        QGraphicsItem::hoverEnterEvent(event);
        return;
    }
    Plasma::Animator *animdriver = Plasma::Animator::self();
    if (d->animHighlightId) {
        animdriver->stopCustomAnimation(d->animHighlightId);
    }
    d->hovering = true;
    d->animHighlightId =
        animdriver->customAnimation(
            10, 240, Plasma::Animator::EaseInCurve, this, "animateHighlight");

    QGraphicsItem::hoverEnterEvent(event);
}

void DesktopToolBox::showToolBox()
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
    int x;
    int y;
    switch (corner()) {
    case TopRight:
        x = (int)boundingRect().left() - maxwidth - iconWidth - 5;
        y = (int)boundingRect().top() + 5;
        break;
    case Top:
        x = (int)boundingRect().center().x() - iconWidth;
        y = (int)boundingRect().top() + iconWidth + 5;
        break;
    case TopLeft:
        x = (int)boundingRect().left() + iconWidth + 5;
        y = (int)boundingRect().top() + 5;
        break;
    case Left:
        x = (int)boundingRect().left() + iconWidth + 5;
        y = (int)boundingRect().center().y() - iconWidth;
        break;
    case Right:
        x = (int)boundingRect().left() - maxwidth - iconWidth - 5;
        y = (int)boundingRect().center().y() - iconWidth;
        break;
    case BottomLeft:
        x = (int)boundingRect().left() + iconWidth + 5;
        y = (int)boundingRect().bottom() - 5;
        break;
    case Bottom:
        x = (int)boundingRect().center().x() - iconWidth;
        y = (int)boundingRect().bottom() - iconWidth - 5;
        break;
    case BottomRight:
    default:
        x = (int)boundingRect().left() - maxwidth - iconWidth - 5;
        y = (int)boundingRect().bottom() - iconWidth - 5;
        break;
    }
    Plasma::Animator *animdriver = Plasma::Animator::self();
    foreach (QGraphicsItem *tool, QGraphicsItem::children()) {
        if (tool == d->toolBacker) {
            continue;
        }

        if (!tool->isEnabled()) {
            if (tool->isVisible()) {
                const int height = static_cast<int>(tool->boundingRect().height());
                animdriver->moveItem(tool, Plasma::Animator::SlideOutMovement,
                                     toolPosition(height));
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

    if (d->animCircleId) {
        animdriver->stopCustomAnimation(d->animCircleId);
    }

    setShowing(true);
    // TODO: 10 and 200 shouldn't be hardcoded here. There needs to be a way to
    // match whatever the time is that moveItem() takes. Same in hoverLeaveEvent().
    d->animCircleId =
        animdriver->customAnimation(10, 240, Plasma::Animator::EaseInCurve, this, "animateCircle");
}

void DesktopToolBox::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    //kDebug() << event->pos() << event->scenePos()
    //         << d->toolBacker->rect().contains(event->scenePos().toPoint());
    if (! d->hovering) {
        QGraphicsItem::hoverLeaveEvent(event);
        return;
    }

    hideToolBox();
    Plasma::Animator *animdriver = Plasma::Animator::self();
    if (d->animHighlightId) {
        animdriver->stopCustomAnimation(d->animHighlightId);
    }
    d->hovering = false;
    d->animHighlightId =
        animdriver->customAnimation(
            10, 240, Plasma::Animator::EaseOutCurve, this, "animateHighlight");

    QGraphicsItem::hoverLeaveEvent(event);
}

void DesktopToolBox::hideToolBox()
{
    if (!showing()) {
        return;
    }

    Plasma::Animator *animdriver = Plasma::Animator::self();
    foreach (QGraphicsItem *tool, QGraphicsItem::children()) {
        if (tool == d->toolBacker) {
            continue;
        }

        const int height = static_cast<int>(tool->boundingRect().height());
        animdriver->moveItem(tool, Plasma::Animator::SlideOutMovement, toolPosition(height));
    }

    if (d->animCircleId) {
        animdriver->stopCustomAnimation(d->animCircleId);
    }

    setShowing(false);
    d->animCircleId =
        animdriver->customAnimation(10, 240, Plasma::Animator::EaseOutCurve, this, "animateCircle");

    if (d->toolBacker) {
        d->toolBacker->hide();
    }
}

void DesktopToolBox::animateCircle(qreal progress)
{
    if (showing()) {
        d->animCircleFrame = size() * progress;
    } else {
        d->animCircleFrame = size() * (1.0 - progress);
    }

    if (progress >= 1) {
        d->animCircleId = 0;
    }

    update();
}

void DesktopToolBox::animateHighlight(qreal progress)
{
    if (d->hovering) {
        d->animHighlightFrame = progress;
    } else {
        d->animHighlightFrame = 1.0 - progress;
    }

    if (progress >= 1) {
        d->animHighlightId = 0;
    }

    update();
}

void DesktopToolBox::toolMoved(QGraphicsItem *item)
{
    //kDebug() << "geometry is now " << static_cast<Plasma::Widget*>(item)->geometry();
    if (!showing() &&
        QGraphicsItem::children().indexOf(static_cast<Plasma::Applet*>(item)) != -1) {
        item->hide();
    }
}

void DesktopToolBox::toggle()
{
    if (showing()) {
        hideToolBox();
    } else {
        showToolBox();
    }
}

} // plasma namespace

#include "desktoptoolbox_p.moc"
