/*
 *   Copyright 2007 Aaron Seigo <aseigo@kde.org>
 *   Copyright 2007 Alexis Ménard <darktears31@gmail.com>
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

#ifndef PLASMA_ANIMATIONDRIVER_H
#define PLASMA_ANIMATIONDRIVER_H

#include <QtCore/QObject>
#include <QtGui/QRegion>
#include <QtGui/QPixmap>

#include <kgenericfactory.h>

#include <plasma/version.h>
#include <plasma/animator.h>

class QGraphicsItem;

namespace Plasma
{

class AnimationDriverPrivate;

class PLASMA_EXPORT AnimationDriver : public QObject
{
    Q_OBJECT

public:
    explicit AnimationDriver(QObject *parent = 0);
    ~AnimationDriver();

    // Parameter definitions
    virtual int animationFps(Plasma::Animator::Animation) const;
    virtual int movementAnimationFps(Plasma::Animator::Movement) const;
    virtual int elementAnimationFps(Plasma::Animator::Animation) const;
    virtual int animationDuration(Plasma::Animator::Animation) const;
    virtual int movementAnimationDuration(Plasma::Animator::Movement) const;
    virtual int elementAnimationDuration(Plasma::Animator::Animation) const;
    virtual Animator::CurveShape animationCurve(Plasma::Animator::Animation) const;
    virtual Animator::CurveShape movementAnimationCurve(Plasma::Animator::Movement) const;
    virtual Animator::CurveShape elementAnimationCurve(Plasma::Animator::Animation) const;

    // Element animations
    virtual QPixmap elementAppear(qreal progress, const QPixmap &pixmap);
    virtual QPixmap elementDisappear(qreal progress, const QPixmap &pixmap);

    // Item animations
    virtual void itemAppear(qreal progress, QGraphicsItem *item);
    virtual void itemDisappear(qreal progress, QGraphicsItem *item);
    virtual void itemActivated(qreal progress, QGraphicsItem *item);

    // Item movements
    virtual void itemSlideIn(qreal progress, QGraphicsItem *item,
                             const QPoint &start, const QPoint &destination);
    virtual void itemSlideOut(qreal progress, QGraphicsItem *item,
                              const QPoint &start, const QPoint &destination);

private:
    AnimationDriverPrivate *const d;
};

} // Plasma namespace

#define K_EXPORT_PLASMA_ANIMATOR(libname, classname) \
K_PLUGIN_FACTORY(factory, registerPlugin<classname>();) \
K_EXPORT_PLUGIN(factory("plasma_animator_" #libname)) \
K_EXPORT_PLUGIN_VERSION(PLASMA_VERSION)

#endif // multiple inclusion guard
