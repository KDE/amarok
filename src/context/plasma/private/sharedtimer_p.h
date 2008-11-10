/*
 *   Copyright 2008 Aaron Seigo <aseigo@kde.org>
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

#ifndef PLASMA_SHAREDTIMER_P_H
#define PLASMA_SHAREDTIMER_P_H

#include <QtCore/QObject>

namespace Plasma
{

class Timer;

class TimerDrive : public QObject
{
    Q_OBJECT

public:
    static TimerDrive *self();
    void registerTimer(const Timer *t, int msec);
    void unregisterTimer(const Timer *t, int msec);

protected:
    void timerEvent(QTimerEvent *event);

private:
    friend class TimerDriveSingleton;
    explicit TimerDrive(QObject *parent = 0);
    ~TimerDrive();
    class Private;
    Private * const d;
};

} // namespace Plasma

#endif

