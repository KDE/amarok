/****************************************************************************************
 * Copyright (c) 2013 Tatjana Gornak <t.gornak@gmail.com>                               *
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

#include "PlaylistRestorer.h"

#include "amarokconfig.h"
#include "core/logger/Logger.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core-impl/playlists/types/file/PlaylistFileSupport.h"
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistController.h"
#include "playlist/PlaylistDock.h"
#include "playlist/PlaylistModelStack.h"
#include "playlistmanager/PlaylistManager.h"

#include <QStandardPaths>

using namespace Playlist;

Restorer::Restorer()
         : m_position( m_tracks )
{
}

void
Restorer::restore( const QUrl &defaultPath )
{
    m_tracks.clear();
    m_playlistToRestore = Playlists::loadPlaylistFile( defaultPath );
    if ( m_playlistToRestore ) // This pointer will be 0 on first startup
    {
        subscribeTo( Playlists::PlaylistPtr::staticCast( m_playlistToRestore ) );
        m_playlistToRestore->triggerTrackLoad();
    }
    else
        runJingle();
}

void
Restorer::runJingle()
{
    DEBUG_BLOCK
    if( AmarokConfig::playFirstRunJingle() )
    {
        QString jingle = QStandardPaths::locate( QStandardPaths::GenericDataLocation, QStringLiteral("amarok/data/first_run_jingle.ogg") );
        The::playlistController()->clear();
        The::playlistController()->insertTrack( 0, CollectionManager::instance()->trackForUrl( QUrl::fromLocalFile(jingle) ) );
        AmarokConfig::setPlayFirstRunJingle( false );
    }
    Q_EMIT restoreFinished();
}

void
Restorer::tracksLoaded(Playlists::PlaylistPtr playlist )
{
    if( m_playlistToRestore == playlist )
    {
        m_tracks = playlist->tracks();
        m_position = m_tracks;
        processTracks();
    }
    else
    {
        // process child playlists
        Meta::TrackList newtracks = playlist->tracks();
        for( Meta::TrackPtr t : newtracks )
            if( t )
                m_position.insert( t );
        processTracks();
    }
}

void
Restorer::processTracks()
{
    while( m_position.hasNext() )
    {
        m_position.next();
        Meta::TrackPtr track = m_position.value();
        if( ! track )
            m_position.remove();
        else if( Playlists::canExpand( track ) )
        {
            Playlists::PlaylistPtr playlist = Playlists::expand( track );
            //expand() can return 0 if the KIO job errors out
            if( playlist )
            {
                m_position.remove();
                subscribeTo( playlist );
                playlist->triggerTrackLoad(); //playlist track loading is on demand.
                // Execution will be envoked after loading of playlist is finished
                return;
           }
        }
    }
    // This code executes only ones after there is no more
    // playlists in m_tracks
    The::playlistController()->insertTracks( 0, m_tracks );
    Actions::instance()->queue( m_playlistToRestore->queue() );

    //Select previously playing track
    const int lastPlayingRow = AmarokConfig::lastPlaying();
    if( lastPlayingRow >= 0 )
        ModelStack::instance()->bottom()->setActiveRow( lastPlayingRow );
    Q_EMIT restoreFinished();
}
