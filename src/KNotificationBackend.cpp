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

#include <KStandardDirs>

#include <QTextDocument> // for Qt::escape()
#include <QTimer>

Amarok::KNotificationBackend::KNotificationBackend()
    : EngineObserver( The::engineController() )
{
    DEBUG_BLOCK

    m_timer = new QTimer( this );
    m_timer->setSingleShot( true );
    connect( m_timer, SIGNAL( timeout() ), this, SLOT( slotShowCurrentTrack() ) );
}

Amarok::KNotificationBackend::~KNotificationBackend()
{
    DEBUG_BLOCK
}

// TODO: Implement
void
Amarok::KNotificationBackend::engineStateChanged( Phonon::State state, Phonon::State /*oldState*/ )
{
    switch( state )
    {
        case Phonon::PlayingState:
        case Phonon::StoppedState:
        case Phonon::PausedState:
        case Phonon::LoadingState:
        case Phonon::ErrorState:
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
Amarok::KNotificationBackend::slotShowCurrentTrack()
{
    DEBUG_BLOCK

    Meta::TrackPtr track = The::engineController()->currentTrack();

    if( track )
    {
        QString text;

        text = "<b>" + Qt::escape( track->prettyName() ) + "</b>";
        if( track->artist() )
        {
            const QString artist = Qt::escape( track->artist()->prettyName() );
            if( !artist.isEmpty() )
                text += i18n( " by <b>%1</b>", artist );
        }
        if( track->album() )
        {
            const QString album = Qt::escape( track->album()->prettyName() );
            if( !album.isEmpty() )
                text += i18n( " on <b>%1</b>", album );
            
            KNotification::event( "trackChange", text, track->album()->image( 80 ) );

            //BAAAD JUJU, this is KDE 4.3 only!
            /*->setTitle( i18n( "Now playing" ) ); */
        }
    }
}

#include "KNotificationBackend.moc"
