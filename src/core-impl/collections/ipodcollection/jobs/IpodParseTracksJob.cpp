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

#include "IpodCollection.h"
#include "IpodMeta.h"
#include "IpodPlaylistProvider.h"
#include "core/logger/Logger.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "core-impl/meta/file/File.h"

#include <QAction>
#include <QDir>
#include <QFileInfo>

#include <gpod/itdb.h>

IpodParseTracksJob::IpodParseTracksJob( IpodCollection *collection )
    : QObject()
    , ThreadWeaver::Job()
    , m_coll( collection )
    , m_aborted( false )
{
}

void IpodParseTracksJob::abort()
{
    m_aborted = true;
}

void
IpodParseTracksJob::run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread)
{
    Q_UNUSED(self);
    Q_UNUSED(thread);
    DEBUG_BLOCK
    Itdb_iTunesDB *itdb = m_coll->m_itdb;
    if( !itdb )
        return; // paranoia

    guint32 trackNumber = itdb_tracks_number( itdb );
    QString operationText = i18nc( "operation when iPod is connected", "Reading iPod tracks" );
    Amarok::Logger::newProgressOperation( this, operationText, trackNumber,
                                                        this, &IpodParseTracksJob::abort );

    Meta::TrackList staleTracks;
    QSet<QString> knownPaths;

    for( GList *tracklist = itdb->tracks; tracklist; tracklist = tracklist->next )
    {
        if( m_aborted )
            break;

        Itdb_Track *ipodTrack = (Itdb_Track *) tracklist->data;
        if( !ipodTrack )
            continue; // paranoia
        // IpodCollection::addTrack() locks and unlocks the m_itdbMutex mutex
        Meta::TrackPtr proxyTrack = m_coll->addTrack( new IpodMeta::Track( ipodTrack ) );
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

    parsePlaylists( staleTracks, knownPaths );
    Q_EMIT endProgressOperation( this );
}

void 
IpodParseTracksJob::defaultBegin(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread)
{
    Q_EMIT started(self);
    ThreadWeaver::Job::defaultBegin(self, thread);
}

void
IpodParseTracksJob::defaultEnd(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread)
{
    ThreadWeaver::Job::defaultEnd(self, thread);
    if (!self->success()) {
        Q_EMIT failed(self);
    }
    Q_EMIT done(self);
}

void
IpodParseTracksJob::parsePlaylists( const Meta::TrackList &staleTracks,
                                    const QSet<QString> &knownPaths )
{
    IpodPlaylistProvider *prov = m_coll->m_playlistProvider;
    if( !prov || m_aborted )
        return;

    if( !staleTracks.isEmpty() )
    {
        prov->m_stalePlaylist = Playlists::PlaylistPtr( new IpodPlaylist( staleTracks,
            i18nc( "iPod playlist name", "Stale tracks" ), m_coll, IpodPlaylist::Stale ) );
        prov->m_playlists << prov->m_stalePlaylist;  // we don't subscribe to this playlist, no need to update database
        Q_EMIT prov->playlistAdded( prov->m_stalePlaylist );
    }

    Meta::TrackList orphanedTracks = findOrphanedTracks( knownPaths );
    if( !orphanedTracks.isEmpty() )
    {
        prov->m_orphanedPlaylist = Playlists::PlaylistPtr( new IpodPlaylist( orphanedTracks,
            i18nc( "iPod playlist name", "Orphaned tracks" ), m_coll, IpodPlaylist::Orphaned ) );
        prov->m_playlists << prov->m_orphanedPlaylist;  // we don't subscribe to this playlist, no need to update database
        Q_EMIT prov->playlistAdded( prov->m_orphanedPlaylist );
    }

    if( !m_coll->m_itdb || m_aborted )
        return;
    for( GList *playlists = m_coll->m_itdb->playlists; playlists; playlists = playlists->next )
    {
        Itdb_Playlist *playlist = (Itdb_Playlist *) playlists->data;
        if( !playlist || itdb_playlist_is_mpl( playlist )  )
            continue; // skip null (?) or master playlists
        Playlists::PlaylistPtr playlistPtr( new IpodPlaylist( playlist, m_coll ) );
        prov->m_playlists << playlistPtr;
        prov->subscribeTo( playlistPtr );
        Q_EMIT prov->playlistAdded( playlistPtr );
    }

    if( !m_aborted && ( prov->m_stalePlaylist || prov->m_orphanedPlaylist ) )
    {
        QString text = i18n( "Stale and/or orphaned tracks detected on %1. You can resolve "
            "the situation using the <b>%2</b> collection action. You can also view the tracks "
            "under the Saved Playlists tab.", m_coll->prettyName(),
            m_coll->m_consolidateAction->text() );
        Amarok::Logger::longMessage( text );
    }
}

Meta::TrackList IpodParseTracksJob::findOrphanedTracks(const QSet< QString >& knownPaths)
{
    gchar *musicDirChar = itdb_get_music_dir( QFile::encodeName( m_coll->mountPoint() ).constData() );
    QString musicDirPath = QFile::decodeName( musicDirChar );
    g_free( musicDirChar );
    musicDirChar = nullptr;

    QStringList trackPatterns;
    for( const QString &suffix : m_coll->supportedFormats() )
    {
        trackPatterns << QStringLiteral( "*.%1" ).arg( suffix );
    }

    Meta::TrackList orphanedTracks;
    QDir musicDir( musicDirPath );
    for( QString subdir : musicDir.entryList( QStringList( QStringLiteral("F??") ), QDir::Dirs | QDir::NoDotAndDotDot ) )
    {
        if( m_aborted )
            return Meta::TrackList();
        subdir = musicDir.absoluteFilePath( subdir ); // make the path absolute
        for( const QFileInfo &info : QDir( subdir ).entryInfoList( trackPatterns ) )
        {
            QString canonPath = info.canonicalFilePath();
            if( knownPaths.contains( canonPath ) )
                continue;  // already in iTunes database
            Meta::TrackPtr track( new MetaFile::Track( QUrl::fromLocalFile( canonPath ) ) );
            orphanedTracks << track;
        }
    }
    return orphanedTracks;
}
