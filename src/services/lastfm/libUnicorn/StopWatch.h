/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
 *      Erik Jaelevik, Last.fm Ltd <erik@last.fm>                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef STOPWATCH_H
#define STOPWATCH_H

#include "UnicornDllExportMacro.h"

#include <QThread>
#include <QMutex>
#include <QDateTime>

class CPlayerConnection;

/*************************************************************************/ /**
    A timer class that counts seconds. It spawns its own thread which just
    sits there and counts. On timeout it notifies whoever's registered with it.
    
    It's probably not very accurate since it just uses sleep intervals to
    count time, but it's accurate enough for our purposes.
******************************************************************************/
class UNICORN_DLLEXPORT StopWatch : public QThread
{
    Q_OBJECT

public:

    /*********************************************************************/ /**
        Ctors
    **************************************************************************/
    StopWatch();
    StopWatch(
        const StopWatch& that);
    StopWatch& operator=(
        const StopWatch& that);

    /*********************************************************************/ /**
        Start counting. Will spawn thread.
    **************************************************************************/
    void
    start();

    /*********************************************************************/ /**
        Stop timer.
    **************************************************************************/
    void
    stop();

    /*********************************************************************/ /**
        Reset timer to zero.
    **************************************************************************/
    void
    reset();

    /*********************************************************************/ /**
        Set the timeout.

        @param[in] nTimeout Timeout in seconds
    **************************************************************************/
    void
    setTimeout(
        int nTimeout);

    /*********************************************************************/ /**
        Returns current time.
    **************************************************************************/
    int
    getTime() { return mnTimer; }

    /*********************************************************************/ /**
        Returns the timeout.
    **************************************************************************/
    int
    getTimeOut() { return mnTimeout; }

    /*********************************************************************/ /**
        A comment on this method would be utterly reundant.
    **************************************************************************/
    bool
    isRunning() { return mState == RUNNING; }

signals:

    /*********************************************************************/ /**
        Emitted when timer value changes.
    **************************************************************************/
    void
    valueChanged(
        int elapsed);

    /*********************************************************************/ /**
        Emitted when timer is reset.
    **************************************************************************/
    void
    timerReset();

    /*********************************************************************/ /**
        Emitted when timeout is changed.
    **************************************************************************/
    void
    timeoutChanged(
        int timeOut);

    /*********************************************************************/ /**
        Emitted when timeout is reached.
    **************************************************************************/
    void
    timeoutReached();


private:

    /*********************************************************************/ /**
        Used by copy ctor and operator=
    **************************************************************************/
    void
    clone(
        const StopWatch& that);

    enum EStopWatchState
    {
        STOPPED,
        RUNNING
    };

    /*********************************************************************/ /**
        QThread run method.
    **************************************************************************/
    virtual void
    run();

    EStopWatchState mState;

    QDateTime mLastTime;

    int mnTotalMs;
    int mnTimer;
    int mnTimeout;

    bool mbTimedOut;

    QMutex mMutex;
};

#endif // STOPWATCH_H
