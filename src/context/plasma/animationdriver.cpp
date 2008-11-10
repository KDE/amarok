/*
 *   Copyright 2007 Aaron Seigo <aseigo@kde.org>
 *             2007 Alexis Ménard <darktears31@gmail.com>
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

#include "animationdriver.h"

#include <QPainter>
#include <QGraphicsItem>

namespace Plasma
{

AnimationDriver::AnimationDriver(QObject *parent)
    : QObject(parent),
      d(0)
{
}

AnimationDriver::~AnimationDriver()
{
}

int AnimationDriver::animationFps(Plasma::Animator::Animation animation) const
{
    Q_UNUSED(animation)
    return 0;
}

int AnimationDriver::movementAnimationFps(Plasma::Animator::Movement movement) const
{
    Q_UNUSED(movement)
    return 40;
}

int AnimationDriver::elementAnimationFps(Plasma::Animator::Animation animation) const
{
    Q_UNUSED(animation)
    return 0;
}

int AnimationDriver::animationDuration(Plasma::Animator::Animation) const
{
    return 200;
}

int AnimationDriver::movementAnimationDuration(Plasma::Animator::Movement movement) const
{
    switch (movement) {
        case Animator::FastSlideInMovement:
        case Animator::FastSlideOutMovement:
            return 150;
            break;
        default:
            break;
    }

    return 250;
}

int AnimationDriver::elementAnimationDuration(Plasma::Animator::Animation) const
{
    return 333;
}

Animator::CurveShape AnimationDriver::animationCurve(Plasma::Animator::Animation) const
{
    return Animator::EaseInOutCurve;
}

Animator::CurveShape AnimationDriver::movementAnimationCurve(Plasma::Animator::Movement) const
{
    return Animator::EaseInOutCurve;
}

Animator::CurveShape AnimationDriver::elementAnimationCurve(Plasma::Animator::Animation) const
{
    return Animator::EaseInOutCurve;
}

QPixmap AnimationDriver::elementAppear(qreal progress, const QPixmap &pixmap)
{
    Q_UNUSED(progress)
    return pixmap;
}

QPixmap AnimationDriver::elementDisappear(qreal progress, const QPixmap &pixmap)
{
    Q_UNUSED(progress)
    QPixmap pix(pixmap.size());
    pix.fill(Qt::transparent);

    return pix;
}

void AnimationDriver::itemAppear(qreal frame, QGraphicsItem *item)
{
    Q_UNUSED(frame)
    Q_UNUSED(item)
}

void AnimationDriver::itemDisappear(qreal frame, QGraphicsItem *item)
{
    Q_UNUSED(frame)
    Q_UNUSED(item)
}

void AnimationDriver::itemActivated(qreal frame, QGraphicsItem *item)
{
    Q_UNUSED(frame)
    Q_UNUSED(item)
}

void AnimationDriver::itemSlideIn(qreal progress, QGraphicsItem *item, const QPoint &start, const QPoint &destination)
{
    double x = start.x() + (destination.x() - start.x()) * progress;
    double y = start.y() + (destination.y() - start.y()) * progress;
    item->setPos(x, y);
}

void AnimationDriver::itemSlideOut(qreal progress, QGraphicsItem *item, const QPoint &start, const QPoint &destination)
{
    //kDebug();
    double x = start.x() + (destination.x() - start.x()) * progress;
    double y = start.y() + (destination.y() - start.y()) * progress;
    item->setPos(x, y);
}

} // Plasma namespace

#include "animationdriver.moc"
