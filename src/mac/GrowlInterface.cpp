/****************************************************************************************
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
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

#include "GrowlInterface.h"

#include "amarokconfig.h"
#include "App.h"
#include "Debug.h"
#include "EngineController.h"
#include "MetaUtility.h" // for secToPrettyTime
#include "TrayIcon.h"

GrowlInterface::GrowlInterface( QString appName ) :
                m_appName( appName )
              ,  EngineObserver( The::engineController() )
{}

void
GrowlInterface::show( Meta::TrackPtr track )
{
    DEBUG_BLOCK
    QString text;
    if( !track || track->playableUrl().isEmpty() )
        text = i18n( "No track playing" );
    else
    {
        text = track->prettyName();
        if( track->artist() && !track->artist()->prettyName().isEmpty() )
            text = track->artist()->prettyName() + " - " + text;
        if( track->album() && !track->album()->prettyName().isEmpty() )
            text += "\n (" + track->album()->prettyName() + ") ";
        else
            text += '\n';
        if( track->length() > 0 )
            text += Meta::msToPrettyTime( track->length() );
    }

    if( text.isEmpty() )
        text =  track->playableUrl().fileName();

    if( text.startsWith( "- " ) ) //When we only have a title tag, _something_ prepends a fucking hyphen. Remove that.
        text = text.mid( 2 );

    if( text.isEmpty() ) //still
        text = i18n("No information available for this track");

    if( App::instance()->trayIcon() )
    {
        App::instance()->trayIcon()->setVisible( true );
        if( track && track->album() )
            App::instance()->trayIcon()->setIcon( QIcon( track->album()->imageWithBorder( 100, 5 ) ) );
        App::instance()->trayIcon()->showMessage( "Amarok", text );
    }

}

void
GrowlInterface::engineNewTrackPlaying()
{
    DEBUG_BLOCK
    Meta::TrackPtr track = The::engineController()->currentTrack();

    unsubscribeFrom( m_currentTrack );
    m_currentTrack = track;
    subscribeTo( track );
    metadataChanged( track );

    show( m_currentTrack );
}

void
GrowlInterface::engineStateChanged( Phonon::State state, Phonon::State oldState )
{
    Q_UNUSED( oldState )
    DEBUG_BLOCK
  //  if( state == Phonon::PausedState )
  //        show( i18n( "Paused" ) );
}


void
GrowlInterface::engineVolumeChanged( int newVolume )
{
  //  volChanged( newVolume );
}

void
GrowlInterface::metadataChanged( Meta::TrackPtr track )
{
    Q_UNUSED( track )
    DEBUG_BLOCK

   // show( m_currentTrack );
}
