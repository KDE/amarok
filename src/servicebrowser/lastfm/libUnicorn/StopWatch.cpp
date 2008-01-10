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
 *   51 Franklin Steeet, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "StopWatch.h"
#include "logger.h"

/******************************************************************************
    CStopWatch
******************************************************************************/
StopWatch::StopWatch() :
    QThread(),
    mState( STOPPED ),
    mnTotalMs( 0 ),
    mnTimer( 0 ),
    mnTimeout( 0 ),
    mbTimedOut( false )
{
}

/******************************************************************************
    CStopWatch copy
******************************************************************************/
StopWatch::StopWatch(
    const StopWatch& that) :
    QThread()
{
    clone(that);
}

/******************************************************************************
    operator=
******************************************************************************/
StopWatch&
StopWatch::operator=(
    const StopWatch& that)
{
    // Check for self-assignment
    if (&that != this)
    {
        clone(that);
    }

    return *this;
}

/******************************************************************************
    Clone
******************************************************************************/
void
StopWatch::clone(
    const StopWatch& that)
{
    QMutexLocker grab(&mMutex);
    mnTotalMs   = that.mnTotalMs;
    mnTimer     = that.mnTimer;
    mnTimeout   = that.mnTimeout;
    mState      = that.mState;
}

/******************************************************************************
    Start
******************************************************************************/
void
StopWatch::start()
{
    mMutex.lock();
    EStopWatchState state = mState;
    mMutex.unlock();

    if (state == RUNNING)
    {
        return;
    }

    mMutex.lock();
    mState = RUNNING;
    mMutex.unlock();

    QThread::start();
}

/******************************************************************************
    Stop
******************************************************************************/
void
StopWatch::stop()
{
    mMutex.lock();
    mState = STOPPED;
    mMutex.unlock();

    // We need to make sure the timer thread has finished since it might call
    // Notify after we've called Stop. Since it's going back to
    // PlayerConnection::onScrobbleTimeout to submit a track, we need to ensure
    // that the track it submits is the same one that it was timing.
    wait();
}

/******************************************************************************
    Reset
******************************************************************************/
void
StopWatch::reset()
{
    mMutex.lock();
    mnTimer = 0;
    mnTotalMs = 0;
    mbTimedOut = false;
    mMutex.unlock();

    emit valueChanged(mnTimer);
    emit timerReset();
}

/******************************************************************************
    setTimeout
******************************************************************************/
void
StopWatch::setTimeout(
    int nTimeout)
{
    mMutex.lock();
    int curTime = mnTimer;
    mMutex.unlock();

    Q_ASSERT( nTimeout > curTime );

    mnTimeout = nTimeout;

    emit timeoutChanged(mnTimeout);
}


/******************************************************************************
    ThreadMain
******************************************************************************/
void
StopWatch::run()
{
    bool bStopped = false;
    mLastTime = QDateTime::currentDateTime();

    do
    {
        int nSleepInterval = 250;
        msleep(nSleepInterval);

        mMutex.lock();

        // Poll for stop every nSleepInterval
        bStopped = mState == STOPPED;

        QDateTime currentTime = QDateTime::currentDateTime();
        int msSpentSleeping = mLastTime.time().msecsTo( currentTime.time() );
        if ( msSpentSleeping >= 1000 )
        {
            mLastTime = currentTime;

            mnTotalMs += msSpentSleeping;
            mnTimer = mnTotalMs / (int)1000;

            /*
            LOGL( 3, "msSpent: " << msSpentSleeping );
            LOGL( 3, "ms: " << mnTotalMs );
            LOGL( 3, "s: " << mnTimer );
            */

            // Poll for timeout every second
            if (!mbTimedOut && mnTimer >= mnTimeout)
            {
                emit timeoutReached();
                mbTimedOut = true;
            }

            emit valueChanged(mnTimer);
        }

        mMutex.unlock();

    } while( !bStopped );
}
