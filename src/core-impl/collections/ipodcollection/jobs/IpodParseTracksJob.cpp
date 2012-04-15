/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#include "IpodParseTracksJob.h"

#include "IpodMeta.h"
#include "IpodPlaylistProvider.h"
#include "core/interfaces/Logger.h"
#include "core/support/Components.h"

#include <gpod/itdb.h>


IpodParseTracksJob::IpodParseTracksJob( IpodCollection *collection )
    : Job()
    , m_coll( collection )
    , m_aborted( false )
{
}

void
IpodParseTracksJob::run()
{
    if( !m_coll || !m_coll.data()->m_itdb )
        return;

    Itdb_iTunesDB *itdb = m_coll.data()->m_itdb;
    guint32 trackNumber = itdb_tracks_number( itdb );
    QString operationText = i18nc( "operation when iPod is connected", "Reading iPod tracks" );
    Amarok::Components::logger()->newProgressOperation( this, operationText, trackNumber,
                                                        this, SLOT(abort()) );

    Meta::TrackList staleTracks;
    QSet<QString> knownPaths;

    for( GList *tracklist = itdb->tracks; tracklist; tracklist = tracklist->next )
    {
        if( m_aborted || !m_coll )
            break;

        Itdb_Track *ipodTrack = (Itdb_Track *) tracklist->data;
        if( !ipodTrack )
            continue; // paranoia
        // IpodCollection::addTrack() locks and unlocks the m_itdbMutex mutex
        Meta::TrackPtr proxyTrack = m_coll.data()->addTrack( new IpodMeta::Track( ipodTrack ) );
        if( proxyTrack )
        {
            QString canonPath = QFileInfo( proxyTrack->playableUrl().toLocalFile() ).canonicalFilePath();
            if( !proxyTrack->isPlayable() )
                staleTracks.append( proxyTrack );
            else if( !canonPath.isEmpty() )  // nonexistent files return empty canonical path
                knownPaths.insert( canonPath );
        }

        incrementProgress();
    }

    IpodPlaylistProvider *provider = m_coll ? m_coll.data()->m_playlistProvider : 0;
    if( provider )
        provider->parseItdbPlaylists( staleTracks, knownPaths );

    emit endProgressOperation( this );
}

void IpodParseTracksJob::abort()
{
    m_aborted = true;
}

#include "IpodParseTracksJob.moc"
