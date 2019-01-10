/***************************************************************************************
 * Copyright (c) 2013 Matěj Laitl <matej@laitl.cz>                                      *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "DelayedDoers.h"

#include "core/support/Debug.h"

#include <phonon/MediaController>
#include <phonon/MediaObject>

DelayedDoer::DelayedDoer( Phonon::MediaObject *mediaObject,
                          const QSet<Phonon::State> &applicableStates )
    : m_mediaObject( mediaObject )
    , m_applicableStates( applicableStates )
{
    Q_ASSERT( mediaObject );
    connect( mediaObject, &Phonon::MediaObject::stateChanged,
             this, &DelayedDoer::slotStateChanged );
    connect( mediaObject, &Phonon::MediaObject::destroyed,
             this, &DelayedDoer::deleteLater );
}

void
DelayedDoer::slotStateChanged( Phonon::State newState )
{
    if( m_applicableStates.contains( newState ) )
    {
        // don't let be called twice, deleteLater() may fire really LATER
        disconnect( m_mediaObject, 0, this, 0 );
        performAction();
        deleteLater();
    }
    else
        debug() << __PRETTY_FUNCTION__ << "newState" << newState << "not applicable, waiting...";
}

DelayedSeeker::DelayedSeeker( Phonon::MediaObject *mediaObject, qint64 seekTo, bool startPaused )
    : DelayedDoer( mediaObject, QSet<Phonon::State>() << Phonon::PlayingState
                                                      << Phonon::BufferingState
                                                      << Phonon::PausedState )
    , m_seekTo( seekTo )
    , m_startPaused( startPaused )
{
}

void
DelayedSeeker::performAction()
{
    m_mediaObject->seek( m_seekTo );
    Q_EMIT trackPositionChanged( m_seekTo, /* userSeek */ true );

    if( !m_startPaused )
        m_mediaObject->play();
}

DelayedTrackChanger::DelayedTrackChanger( Phonon::MediaObject *mediaObject,
                                          Phonon::MediaController *mediaController,
                                          int trackNumber, qint64 seekTo, bool startPaused )
    : DelayedSeeker( mediaObject, seekTo, startPaused )
    , m_mediaController( mediaController )
    , m_trackNumber( trackNumber )
{
    Q_ASSERT( mediaController );
    connect( mediaController, &QObject::destroyed, this, &QObject::deleteLater );
    Q_ASSERT( trackNumber > 0 );
}

void
DelayedTrackChanger::performAction()
{
    m_mediaController->setCurrentTitle( m_trackNumber );
    if( m_seekTo )
    {
        m_mediaObject->seek( m_seekTo );
        Q_EMIT trackPositionChanged( m_seekTo, /* userSeek */ true );
    }

    if( !m_startPaused )
        m_mediaObject->play();
}
