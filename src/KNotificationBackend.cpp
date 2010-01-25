/****************************************************************************************
 * Copyright (c) 2009 Kevin Funk <krf@electrostorm.net>                                 *
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

#include "KNotificationBackend.h"

#include "Amarok.h"
#include "Debug.h"
#include "EngineController.h"
#include "meta/Meta.h"

#include <QTimer>


namespace Amarok
{
    KNotificationBackend*
    KNotificationBackend::s_instance = 0;

    KNotificationBackend*
    KNotificationBackend::instance()
    {
        return s_instance ? s_instance : s_instance = new KNotificationBackend();
    }

    void
    KNotificationBackend::destroy()
    {
        if( s_instance ) { delete s_instance; s_instance = 0; }
    }
}

Amarok::KNotificationBackend::KNotificationBackend()
    : EngineObserver( The::engineController() )
    , m_notify( 0 )
    , m_enabled( false )
{
    DEBUG_BLOCK

    m_timer = new QTimer( this );
    m_timer->setSingleShot( true );
    connect( m_timer, SIGNAL( timeout() ), this, SLOT( showCurrentTrack() ) );
}

Amarok::KNotificationBackend::~KNotificationBackend()
{
    DEBUG_BLOCK

    if (m_notify)
        delete m_notify;
}

void
Amarok::KNotificationBackend::engineStateChanged( Phonon::State state, Phonon::State /*oldState*/ )
{
    switch( state )
    {
        case Phonon::PlayingState:
        case Phonon::StoppedState:
        case Phonon::PausedState:
        case Phonon::LoadingState:
            break;
        case Phonon::ErrorState:
            if( m_timer->isActive() )
                m_timer->stop(); // Do not notify if track cannot be played
        case Phonon::BufferingState:
            break;
    }
}

void
Amarok::KNotificationBackend::engineNewTrackPlaying()
{
    DEBUG_BLOCK

    m_timer->start( 3000 ); // Wait some time to display the correct cover
}

void
Amarok::KNotificationBackend::showCurrentTrack() // slot
{
    DEBUG_BLOCK

    if( !m_enabled )
        return;

    Meta::TrackPtr track = The::engineController()->currentTrack();
    if( track )
    {
        if( m_notify )
            m_notify->close(); // Close old notification when switching quickly between tracks

        m_notify = new KNotification( "trackChange" );

        if( track->album() )
            m_notify->setPixmap( track->album()->imageWithBorder( 80 ) );

        m_notify->setTitle( i18n( "Now playing" ) );

        m_notify->setText( Amarok::prettyNowPlaying() );
        m_notify->sendEvent();
    }
}

#include "KNotificationBackend.moc"
