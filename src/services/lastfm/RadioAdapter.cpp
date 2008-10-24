/***************************************************************************
 * copyright            : (C) 2007 Shane King <kde@dontletsstart.com>      *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "RadioAdapter.h"
#include "Amarok.h"
#include "core/Radio.h"
#include "LastFmService.h"
#include "LastFmSettings.h"
#include "libUnicorn/WebService.h"

#include "statusbar_ng/StatusBar.h"

RadioAdapter::RadioAdapter( QObject *parent, const QString &username, const QString &password )
    : QObject( parent ), m_radio( new Radio( this ) )
{
    The::webService()->setUsername( username );
    The::webService()->setPassword( password );
    m_radio->init( username, password, APP_VERSION );

    connect( m_radio, SIGNAL( error( RadioError, const QString& ) ), this, SLOT( error( RadioError, const QString& ) ) );
}


RadioAdapter::~RadioAdapter()
{
}


void
RadioAdapter::play( const LastFm::TrackPtr &track )
{
    if( track != m_currentTrack )
    {
        bool newStation = The::currentUser().resumeStation() != track->uidUrl();
        m_currentTrack = track;
        emit haveTrack( true );
        if( newStation || The::radio().state() != State_Stopped && The::radio().state() != State_Handshaken )
            The::radio().playStation( track->uidUrl() );
        else
            The::radio().resumeStation();
    }
}


void
RadioAdapter::next()
{
    if( m_currentTrack )
    {
        The::audioController().loadNext();
    }
}


void
RadioAdapter::stop()
{
    if( m_currentTrack )
    {
        m_currentTrack->setTrackInfo( TrackInfo() ); // will emit an empty playable url, thus moving to next track in playlist
        m_currentTrack = 0;
        emit haveTrack( false );
    }
}


void
RadioAdapter::error( RadioError errorCode, const QString& message )
{
    The::statusBar()->longMessage( i18nc("Last.fm: errorMessage", "%1: %2 Error code: %3", "Last.fm", message, errorCode), StatusBar::Error );
    stop();
}


namespace The
{
    Radio &radio()
    {
        return *lastFmService()->radio()->m_radio;
    }
}
