/****************************************************************************************
 * Copyright (c) 2011 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#include "PlaylistFile.h"

#include "core/support/Debug.h"
#include "core-impl/playlists/types/file/PlaylistFileLoaderJob.h"
#include "playlistmanager/file/PlaylistFileProvider.h"
#include "playlistmanager/PlaylistManager.h"

#include <QUrl>
#include <QDir>

#include <ThreadWeaver/Queue>

using namespace Playlists;

PlaylistFile::PlaylistFile( const QUrl &url, PlaylistProvider *provider )
             : Playlist()
             , m_provider( provider )
             , m_url( url )
             , m_tracksLoaded( false )
             , m_name( m_url.fileName() )
             , m_relativePaths( false )
             , m_loadingDone( 0 )
{
}

void
PlaylistFile::saveLater()
{
    PlaylistFileProvider *fileProvider = qobject_cast<PlaylistFileProvider *>( m_provider );
    if( !fileProvider )
        return;

    fileProvider->saveLater( PlaylistFilePtr( this ) );
}

void
PlaylistFile::triggerTrackLoad()
{
    if( m_tracksLoaded )
    {
        notifyObserversTracksLoaded();
        return;
    }
    PlaylistFileLoaderJob *worker = new PlaylistFileLoaderJob( PlaylistFilePtr( this ) );
    ThreadWeaver::Queue::instance()->enqueue( QSharedPointer<ThreadWeaver::Job>(worker) );
    if ( !isLoadingAsync() )
        m_loadingDone.acquire(); // after loading is finished worker will release semapore
}

bool
PlaylistFile::isWritable() const
{
    if( m_url.isEmpty() )
        return false;

    return QFileInfo( m_url.path() ).isWritable();
}

int
PlaylistFile::trackCount() const
{
    if( m_tracksLoaded )
        return m_tracks.count();
    else
        return -1;
}

void
PlaylistFile::addTrack( const Meta::TrackPtr &track, int position )
{
    if( !track ) // playlists might contain invalid tracks. see BUG: 303056
        return;

    int trackPos = position < 0 ? m_tracks.count() : position;
    if( trackPos > m_tracks.count() )
        trackPos = m_tracks.count();
    m_tracks.insert( trackPos, track );
    // set in case no track was in the playlist before
    m_tracksLoaded = true;

    notifyObserversTrackAdded( track, trackPos );

    if( !m_url.isEmpty() )
        saveLater();
}

void
PlaylistFile::removeTrack( int position )
{
    if( position < 0 || position >= m_tracks.count() )
        return;

    m_tracks.removeAt( position );

    notifyObserversTrackRemoved( position );

    if( !m_url.isEmpty() )
        saveLater();
}

bool
PlaylistFile::save( bool relative )
{
    m_relativePaths = relative;
    QMutexLocker locker( &m_saveLock );

    //if the location is a directory append the name of this playlist.
    if( m_url.fileName().isNull() )
    {    m_url = m_url.adjusted(QUrl::RemoveFilename);
         m_url.setPath(m_url.path() + name());
    }
    QFile file( m_url.path() );

    if( !file.open( QIODevice::WriteOnly ) )
    {
        warning() << QStringLiteral( "Cannot write playlist (%1)." ).arg( file.fileName() )
                  << file.errorString();
        return false;
    }

    savePlaylist( file );
    file.close();
    return true;
}

void
PlaylistFile::setName( const QString &name )
{
    //can't save to a new file if we don't know where.
    if( !m_url.isEmpty() && !name.isEmpty() )
    {
        QString exten = QStringLiteral( ".%1" ).arg(extension());
        m_url = m_url.adjusted(QUrl::RemoveFilename);
        m_url.setPath(m_url.path() + name + ( name.endsWith( exten, Qt::CaseInsensitive ) ? QLatin1String("") : exten ));
    }
}

void
PlaylistFile::addProxyTrack( const Meta::TrackPtr &proxyTrack )
{
    m_tracks << proxyTrack;
    notifyObserversTrackAdded( m_tracks.last(), m_tracks.size() - 1 );
}

QUrl
PlaylistFile::getAbsolutePath( const QUrl &url )
{
    QUrl absUrl = url;

    if( url.scheme().isEmpty() )
        absUrl.setScheme( QStringLiteral( "file" ) );

    if( !absUrl.isLocalFile() )
        return url;

    if( !url.path().startsWith( QLatin1Char('/') ) )
    {
        m_relativePaths = true;
        // example: url = QUrl( "file://../tunes/tune.ogg" )
        absUrl = m_url.adjusted(QUrl::RemoveFilename); // file:///playlists/
        absUrl = absUrl.adjusted(QUrl::StripTrailingSlash);
        absUrl.setPath( absUrl.path() + QLatin1Char('/') + url.path() );
        absUrl.setPath( QDir::cleanPath(absUrl.path()) ); // file:///playlists/tunes/tune.ogg
    }
    return absUrl;
}

QString
PlaylistFile::trackLocation( const Meta::TrackPtr &track ) const
{
    QUrl path = track->playableUrl();
    if( path.isEmpty() ) // track is metaproxy or something similar
    {
        QUrl uidPath = QUrl( track->uidUrl() );
        if( !uidPath.isLocalFile() )
            return uidPath.toString( QUrl::FullyEncoded );
        return track->uidUrl();
    }

    if( !m_relativePaths || m_url.isEmpty() || !path.isLocalFile() || !m_url.isLocalFile() )
    {
        return path.toString( QUrl::FullyEncoded );
    }

    QDir playlistDir( m_url.adjusted(QUrl::RemoveFilename).path() );
    return playlistDir.relativeFilePath( path.path() );
}
