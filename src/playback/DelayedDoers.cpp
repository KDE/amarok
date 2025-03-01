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

DelayedDoer::DelayedDoer( )
{
    /*connect( mediaObject, &Phonon::MediaObject::stateChanged,
             this, &DelayedDoer::slotStateChanged );
    connect( mediaObject, &Phonon::MediaObject::destroyed,
             this, &DelayedDoer::deleteLater );*/
}

void
DelayedDoer::slotStateChanged( int newState )
{
    if( 1) //m_applicableStates.contains( newState ) )
    {
        // don't let be called twice, deleteLater() may fire really LATER
        //disconnect( m_mediaObject, nullptr, this, nullptr );
        performAction();
        deleteLater();
    }
    else
        debug() << __PRETTY_FUNCTION__ << "newState" << newState << "not applicable, waiting...";
}

DelayedSeeker::DelayedSeeker( qint64 seekTo, bool startPaused )
    : DelayedDoer( )
    , m_seekTo( seekTo )
    , m_startPaused( startPaused )
{
}

void
DelayedSeeker::performAction()
{
    // TODO m_mediaObject->seek( m_seekTo );
    Q_EMIT trackPositionChanged( m_seekTo, /* userSeek */ true );

  //  if( !m_startPaused )
//  TODO      m_mediaObject->play();
}

DelayedTrackChanger::DelayedTrackChanger( int trackNumber, qint64 seekTo, bool startPaused )
    : DelayedSeeker( seekTo, startPaused )
    , m_trackNumber( trackNumber )
{
    Q_ASSERT( trackNumber > 0 );
}

void
DelayedTrackChanger::performAction()
{
// TODO    m_mediaController->setCurrentTitle( m_trackNumber );
    if( m_seekTo )
    {
// TODO        m_mediaObject->seek( m_seekTo );
        Q_EMIT trackPositionChanged( m_seekTo, /* userSeek */ true );
    }

/*    if( !m_startPaused )
        m_mediaObject->play(); */ //TODO
}
